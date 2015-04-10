/*!
 *  Copyright (C) 2011-2015  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "network/ODClient.h"
#include "network/ODServer.h"
#include "network/ODPacket.h"
#include "network/ServerNotification.h"
#include "render/ODFrameListener.h"
#include "render/RenderManager.h"
#include "network/ChatMessage.h"
#include "gamemap/GameMap.h"
#include "game/Seat.h"
#include "game/Player.h"
#include "game/Research.h"
#include "entities/MapLight.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/CreatureSound.h"
#include "entities/Tile.h"
#include "ODApplication.h"
#include "rooms/RoomTreasury.h"
#include "entities/TreasuryObject.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/Weapon.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "modes/ModeManager.h"
#include "modes/MenuModeConfigureSeats.h"
#include "modes/GameMode.h"
#include "sound/MusicPlayer.h"
#include "sound/SoundEffectsManager.h"
#include "camera/CameraManager.h"

#include <boost/lexical_cast.hpp>

#include <string>

template<> ODClient* Ogre::Singleton<ODClient>::msSingleton = nullptr;

ODClient::ODClient() :
    ODSocketClient(),
    mIsPlayerConfig(false)
{
}

ODClient::~ODClient()
{
}

void ODClient::processClientSocketMessages(GameMap& gameMap)
{
    gameMap.processDeletionQueues();
    // If we receive message for a new turn, after processing every message,
    // we will refresh what is needed
    // We loop until no more data is available
    while(isConnected() && processOneClientSocketMessage());
}

bool ODClient::processOneClientSocketMessage()
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();

    GameMap* gameMap = frameListener->getClientGameMap();
    if (!gameMap)
        return false;

    if(!isDataAvailable())
        return false;

    // Get a reference to the LogManager
    LogManager& logManager = LogManager::getSingleton();

    ODPacket packetReceived;

    // Check if data available
    ODComStatus comStatus = recv(packetReceived);
    if(comStatus != ODComStatus::OK)
    {
        std::string message = getPlayer() ? getPlayer()->getNick() + " disconnected from the server." : "A player disconnected.";
        // Place an event in the queue to inform the user about the disconnection.
        addEventMessage(new EventMessage(message));
        // TODO : try to reconnect to the server
        return false;
    }

    ServerNotificationType serverCommand;
    OD_ASSERT_TRUE(packetReceived >> serverCommand);

    switch(serverCommand)
    {
        case ServerNotificationType::loadLevel:
        {
            std::string odVersion;
            OD_ASSERT_TRUE(packetReceived >> odVersion);

            // Map
            int32_t mapSizeX;
            int32_t mapSizeY;
            OD_ASSERT_TRUE(packetReceived >> mapSizeX);
            OD_ASSERT_TRUE(packetReceived >> mapSizeY);
            if (!gameMap->createNewMap(mapSizeX, mapSizeY))
                return false;

            // Map infos
            std::string str;
            OD_ASSERT_TRUE(packetReceived >> mLevelFilename);
            gameMap->setLevelName(mLevelFilename);
            OD_ASSERT_TRUE(packetReceived >> str);
            gameMap->setLevelDescription(str);
            OD_ASSERT_TRUE(packetReceived >> str);
            gameMap->setLevelMusicFile(str);
            OD_ASSERT_TRUE(packetReceived >> str);
            gameMap->setLevelFightMusicFile(str);

            OD_ASSERT_TRUE(packetReceived >> str);
            if(str.empty())
                str = ConfigManager::DEFAULT_TILESET_NAME;

            gameMap->setTileSetName(str);

            int32_t nb;
            // Creature definitions
            OD_ASSERT_TRUE(packetReceived >> nb);
            while(nb > 0)
            {
                --nb;
                CreatureDefinition* def = new CreatureDefinition();
                OD_ASSERT_TRUE(packetReceived >> def);
                gameMap->addClassDescription(def);
            }

            // Weapons
            OD_ASSERT_TRUE(packetReceived >> nb);
            while(nb > 0)
            {
                --nb;
                Weapon* def = new Weapon();
                OD_ASSERT_TRUE(packetReceived >> def);
                gameMap->addWeapon(def);
            }

            // Seats
            OD_ASSERT_TRUE(packetReceived >> nb);
            while(nb > 0)
            {
                --nb;
                Seat* seat = new Seat(gameMap);
                OD_ASSERT_TRUE(packetReceived >> seat);
                if(!gameMap->addSeat(seat))
                {
                    OD_ASSERT_TRUE(false);
                    delete seat;
                }
            }

            // Tiles
            OD_ASSERT_TRUE(packetReceived >> nb);
            while(nb > 0)
            {
                --nb;
                Tile* tile = gameMap->tileFromPacket(packetReceived);
                tile->setType(TileType::gold);
                tile->setTileVisual(TileVisual::goldFull);
            }
            OD_ASSERT_TRUE(packetReceived >> nb);
            while(nb > 0)
            {
                --nb;
                Tile* tile = gameMap->tileFromPacket(packetReceived);
                tile->setType(TileType::rock);
                tile->setTileVisual(TileVisual::rockFull);
            }
            gameMap->setAllFullnessAndNeighbors();

            ODPacket packSend;
            packSend << ClientNotificationType::levelOK;
            send(packSend);
            break;
        }

        case ServerNotificationType::pickNick:
        {
            ServerMode serverMode;
            OD_ASSERT_TRUE(packetReceived >> serverMode);

            ODPacket packSend;
            const std::string& nick = gameMap->getLocalPlayerNick();
            packSend << ClientNotificationType::setNick << nick;
            send(packSend);

            // We can proceed to configure seat level
            switch(serverMode)
            {
                case ServerMode::ModeGameSinglePlayer:
                case ServerMode::ModeGameMultiPlayer:
                case ServerMode::ModeGameLoaded:
                    frameListener->getModeManager()->requestConfigureSeatsMode(true);
                    break;
                case ServerMode::ModeEditor:
                    break;
                default:
                    OD_ASSERT_TRUE_MSG(false,"Unknown server mode=" + Ogre::StringConverter::toString(static_cast<int32_t>(serverMode)));
                    break;
            }
            // If we are watching a replay, we force stopping the processing loop to
            // allow changing mode (because there is no synchronization as there is no server)
            if(getSource() == ODSource::file)
                return false;
            break;
        }

        case ServerNotificationType::playerConfigChange:
        {
            if(frameListener->getModeManager()->getCurrentModeType() != ModeManager::ModeType::MENU_CONFIGURE_SEATS)
                break;

            MenuModeConfigureSeats* mode = static_cast<MenuModeConfigureSeats*>(frameListener->getModeManager()->getCurrentMode());
            mode->activatePlayerConfig();
        }

        case ServerNotificationType::seatConfigurationRefresh:
        {
            if(frameListener->getModeManager()->getCurrentModeType() != ModeManager::ModeType::MENU_CONFIGURE_SEATS)
                break;

            MenuModeConfigureSeats* mode = static_cast<MenuModeConfigureSeats*>(frameListener->getModeManager()->getCurrentMode());
            mode->refreshSeatConfiguration(packetReceived);
            break;
        }

        case ServerNotificationType::addPlayers:
        {
            if(frameListener->getModeManager()->getCurrentModeType() != ModeManager::ModeType::MENU_CONFIGURE_SEATS)
            {
                OD_ASSERT_TRUE_MSG(false, "Wrong mode " + Ogre::StringConverter::toString(frameListener->getModeManager()->getCurrentModeType()));
                break;
            }

            MenuModeConfigureSeats* mode = static_cast<MenuModeConfigureSeats*>(frameListener->getModeManager()->getCurrentMode());
            uint32_t nbPlayers;
            OD_ASSERT_TRUE(packetReceived >> nbPlayers);
            for(uint32_t i = 0; i < nbPlayers; ++i)
            {
                std::string nick;
                int32_t id;
                OD_ASSERT_TRUE(packetReceived >> nick >> id);
                mode->addPlayer(nick, id);
                addEventMessage(new EventMessage(nick + " is now connected."));
            }
            break;
        }

        case ServerNotificationType::removePlayers:
        {
            if(frameListener->getModeManager()->getCurrentModeType() != ModeManager::ModeType::MENU_CONFIGURE_SEATS)
            {
                OD_ASSERT_TRUE_MSG(false, "Wrong mode " + Ogre::StringConverter::toString(frameListener->getModeManager()->getCurrentModeType()));
                break;
            }

            MenuModeConfigureSeats* mode = static_cast<MenuModeConfigureSeats*>(frameListener->getModeManager()->getCurrentMode());
            uint32_t nbPlayers;
            OD_ASSERT_TRUE(packetReceived >> nbPlayers);
            for(uint32_t i = 0; i < nbPlayers; ++i)
            {
                int32_t id;
                OD_ASSERT_TRUE(packetReceived >> id);
                mode->removePlayer(id);
            }
            break;
        }

        case ServerNotificationType::clientAccepted:
        {
            int32_t nbPlayers;
            OD_ASSERT_TRUE(packetReceived >> ODApplication::turnsPerSecond);

            OD_ASSERT_TRUE(packetReceived >> nbPlayers);
            for(int i = 0; i < nbPlayers; ++i)
            {
                std::string nick;
                int32_t playerId;
                int32_t seatId;
                int32_t teamId;
                OD_ASSERT_TRUE(packetReceived >> nick >> playerId >> seatId >> teamId);
                Player *tempPlayer = new Player(gameMap, playerId);
                tempPlayer->setNick(nick);
                gameMap->addPlayer(tempPlayer);

                Seat* seat = gameMap->getSeatById(seatId);
                OD_ASSERT_TRUE(seat != nullptr);
                seat->setPlayer(tempPlayer);
                seat->setTeamId(teamId);
            }
            break;
        }

        case ServerNotificationType::clientRejected:
        {
            // If should be in seat configuration. If we are rejected, we regress mode
            ModeManager::ModeType modeType = frameListener->getModeManager()->getCurrentModeType();
            OD_ASSERT_TRUE_MSG(modeType == ModeManager::ModeType::MENU_CONFIGURE_SEATS, "Wrong mode type="
                + Ogre::StringConverter::toString(static_cast<int>(modeType)));
            if(modeType != ModeManager::ModeType::MENU_CONFIGURE_SEATS)
                break;

            MenuModeConfigureSeats* mode = static_cast<MenuModeConfigureSeats*>(frameListener->getModeManager()->getCurrentMode());
            mode->goBack();
            break;
        }

        case ServerNotificationType::startGameMode:
        {
            int seatId;
            OD_ASSERT_TRUE(packetReceived >> seatId);
            const std::vector<Player*>& players = gameMap->getPlayers();
            for(Player* player : players)
            {
                if(player->getSeat()->getId() == seatId)
                {
                    setPlayer(player);
                    gameMap->setLocalPlayer(player);
                }
            }

            ServerMode serverMode;
            OD_ASSERT_TRUE(packetReceived >> serverMode);

            // Now that the we have received all needed information, we can launch the requested mode
            switch(serverMode)
            {
                case ServerMode::ModeGameSinglePlayer:
                case ServerMode::ModeGameMultiPlayer:
                case ServerMode::ModeGameLoaded:
                    frameListener->getModeManager()->requestGameMode(true);
                    break;
                case ServerMode::ModeEditor:
                    frameListener->getModeManager()->requestEditorMode(true);
                    break;
                default:
                    OD_ASSERT_TRUE_MSG(false,"Unknown server mode=" + Ogre::StringConverter::toString(static_cast<int32_t>(serverMode)));
            }

            Seat *tempSeat = gameMap->getSeatById(seatId);
            OD_ASSERT_TRUE_MSG(tempSeat != nullptr, "seatId=" + Ogre::StringConverter::toString(seatId));

            // We reset the renderer
            RenderManager::getSingleton().clearRenderer();
            // Move camera to starting position
            Ogre::Real startX = static_cast<Ogre::Real>(tempSeat->mStartingX);
            Ogre::Real startY = static_cast<Ogre::Real>(tempSeat->mStartingY);
            // We make the temple appear in the center of the game view
            startY = startY - 7.0f;
            // Bound check
            if (startY <= 0.0)
                startY = 0.0;

            frameListener->setCameraPosition(Ogre::Vector3(startX, startY, MAX_CAMERA_Z));
            // If we are watching a replay, we force stopping the processing loop to
            // allow changing mode (because there is no synchronization as there is no server)
            if(getSource() == ODSource::file)
                return false;
            break;
        }

        case ServerNotificationType::chat:
        {
            int32_t seatId;
            std::string chatMsg;
            OD_ASSERT_TRUE(packetReceived >> seatId >> chatMsg);
            Seat* seat = gameMap->getSeatById(seatId);
            Player* player = seat ? seat->getPlayer() : nullptr;
            addChatMessage(new ChatMessage(player, chatMsg));
            break;
        }

        case ServerNotificationType::chatServer:
        {
            std::string evtMsg;
            EventShortNoticeType type;
            OD_ASSERT_TRUE(packetReceived >> evtMsg >> type);
            addEventMessage(new EventMessage(evtMsg, type));
            break;
        }

        case ServerNotificationType::newMap:
        {
            gameMap->clearAll();
            break;
        }

        case ServerNotificationType::addClass:
        {
            CreatureDefinition *tempClass = new CreatureDefinition;
            OD_ASSERT_TRUE(packetReceived >> tempClass);
            gameMap->addClassDescription(tempClass);
            break;
        }

        case ServerNotificationType::addEntity:
        {
            GameEntity* entity = GameEntity::getGameEntityFromPacket(gameMap, packetReceived);
            if(entity == nullptr)
                break;
            entity->addToGameMap();
            entity->createMesh();
            entity->restoreEntityState();
            break;
        }

        case ServerNotificationType::removeEntity:
        {
            GameEntityType entityType;
            std::string entityName;
            OD_ASSERT_TRUE(packetReceived >> entityType >> entityName);
            GameEntity* entity = gameMap->getEntityFromTypeAndName(entityType, entityName);
            OD_ASSERT_TRUE_MSG(entity != nullptr, "entityType=" + Ogre::StringConverter::toString(static_cast<int32_t>(entityType)) + ", entityName=" + entityName);
            if(entity == nullptr)
                break;

            entity->removeFromGameMap();
            entity->deleteYourself();
            break;
        }

        case ServerNotificationType::turnStarted:
        {
            int64_t turnNum;
            OD_ASSERT_TRUE(packetReceived >> turnNum);
            logManager.logMessage("Client (" + getPlayer()->getNick() + ") received turnStarted="
                + boost::lexical_cast<std::string>(turnNum));
            gameMap->setTurnNumber(turnNum);

            // We acknowledge the new turn to the server so that he knows we are
            // ready for next one
            ODPacket packSend;
            packSend << ClientNotificationType::ackNewTurn << turnNum;
            send(packSend);

            // For the first turn, we stop processing events because we want the gamemap to
            // be initialized
            if(turnNum == 0)
                return false;

            break;
        }

        case ServerNotificationType::animatedObjectAddDestination:
        {
            std::string objName;
            Ogre::Vector3 vect;
            OD_ASSERT_TRUE(packetReceived >> objName >> vect);
            MovableGameEntity *tempAnimatedObject = gameMap->getAnimatedObject(objName);
            OD_ASSERT_TRUE_MSG(tempAnimatedObject != nullptr, "objName=" + objName);
            if (tempAnimatedObject != nullptr)
                tempAnimatedObject->addDestination(vect.x, vect.y, vect.z);

            break;
        }

        case ServerNotificationType::animatedObjectClearDestinations:
        {
            std::string objName;
            OD_ASSERT_TRUE(packetReceived >> objName);
            MovableGameEntity *tempAnimatedObject = gameMap->getAnimatedObject(objName);
            OD_ASSERT_TRUE_MSG(tempAnimatedObject != nullptr, "objName=" + objName);
            if (tempAnimatedObject != nullptr)
                tempAnimatedObject->clearDestinations();

            break;
        }

        case ServerNotificationType::entityPickedUp:
        {
            int seatId;
            GameEntityType entityType;
            std::string entityName;
            OD_ASSERT_TRUE(packetReceived >> seatId >> entityType >> entityName);
            Player *tempPlayer = gameMap->getPlayerBySeatId(seatId);
            OD_ASSERT_TRUE_MSG(tempPlayer != nullptr, "seatId=" + Ogre::StringConverter::toString(seatId));
            if(tempPlayer == nullptr)
                break;

            GameEntity* entity = gameMap->getEntityFromTypeAndName(entityType, entityName);
            OD_ASSERT_TRUE_MSG(entity != nullptr, "entityType=" + Ogre::StringConverter::toString(static_cast<int32_t>(entityType)) + ", entityName=" + entityName);
            if(entity == nullptr)
                break;

            tempPlayer->pickUpEntity(entity);
            break;
        }

        case ServerNotificationType::entityDropped:
        {
            int seatId;
            OD_ASSERT_TRUE(packetReceived >> seatId);
            OD_ASSERT_TRUE_MSG(gameMap->getLocalPlayer()->getSeat()->getId() == seatId, "seatId=" + Ogre::StringConverter::toString(seatId));
            if (gameMap->getLocalPlayer()->getSeat()->getId() != seatId)
                break;

            Tile* tile = gameMap->tileFromPacket(packetReceived);
            if(tile == nullptr)
                break;

            OD_ASSERT_TRUE(gameMap->getLocalPlayer()->dropHand(tile) != nullptr);
            break;
        }

        case ServerNotificationType::entitySlapped:
        {
            RenderManager::getSingleton().entitySlapped();
            break;
        }

        case ServerNotificationType::setObjectAnimationState:
        {
            std::string objName;
            std::string animState;
            bool loop;
            bool shouldSetWalkDirection;
            OD_ASSERT_TRUE(packetReceived >> objName >> animState
                >> loop >> shouldSetWalkDirection);
            MovableGameEntity *obj = gameMap->getAnimatedObject(objName);
            OD_ASSERT_TRUE_MSG(obj != nullptr, "objName=" + objName + ", state=" + animState);
            if (obj == nullptr)
                break;

            if(shouldSetWalkDirection)
            {
                Ogre::Vector3 walkDirection;
                OD_ASSERT_TRUE(packetReceived >> walkDirection);
                obj->setWalkDirection(walkDirection);
            }

            obj->setAnimationState(animState, loop);
            break;
        }

        case ServerNotificationType::setMoveSpeed:
        {
            std::string objName;
            double moveSpeed;
            OD_ASSERT_TRUE(packetReceived >> objName >> moveSpeed);
            MovableGameEntity *obj = gameMap->getAnimatedObject(objName);
            OD_ASSERT_TRUE_MSG(obj != nullptr, "objName=" + objName + ", moveSpeed=" + Ogre::StringConverter::toString(moveSpeed));
            if (obj == nullptr)
                break;

            obj->setMoveSpeed(moveSpeed);
            break;
        }

        case ServerNotificationType::refreshPlayerSeat:
        {
            Seat tmpSeat(gameMap);
            std::string goalsString;
            OD_ASSERT_TRUE(packetReceived >> &tmpSeat >> goalsString);
            getPlayer()->getSeat()->refreshFromSeat(&tmpSeat);
            refreshMainUI(goalsString);
            break;
        }

        case ServerNotificationType::creatureRefresh:
        {
            std::string name;
            OD_ASSERT_TRUE(packetReceived >> name);
            Creature* creature = gameMap->getCreature(name);
            OD_ASSERT_TRUE_MSG(creature != nullptr, "name=" + name);
            if(creature == nullptr)
                break;

            creature->refreshCreature(packetReceived);
            break;
        }

        case ServerNotificationType::playerFighting:
        {
            int32_t playerFightingId;
            OD_ASSERT_TRUE(packetReceived >> playerFightingId);
            if(getPlayer()->getId() == playerFightingId)
            {
                addEventMessage(new EventMessage("You are under attack!", EventShortNoticeType::majorGameEvent));
            }
            else
            {
                addEventMessage(new EventMessage("An ally is under attack!", EventShortNoticeType::majorGameEvent));
            }

            std::string fightMusic = gameMap->getLevelFightMusicFile();
            if (fightMusic.empty())
                break;
            MusicPlayer::getSingleton().play(fightMusic);
            break;
        }

        case ServerNotificationType::playerNoMoreFighting:
        {
            MusicPlayer::getSingleton().play(gameMap->getLevelMusicFile());
            break;
        }

        case ServerNotificationType::setEntityOpacity:
        {
            std::string entityName;
            float opacity;
            OD_ASSERT_TRUE(packetReceived >> entityName >> opacity);

            RenderedMovableEntity* entity = gameMap->getRenderedMovableEntity(entityName);
            OD_ASSERT_TRUE_MSG(entity != nullptr, "entityName=" + entityName);
            if(entity == nullptr)
                break;

            entity->setMeshOpacity(opacity);
            break;
        }

        case ServerNotificationType::playSpatialSound:
        {
            SoundEffectsManager::InterfaceSound soundType;
            int xPos;
            int yPos;
            OD_ASSERT_TRUE(packetReceived >> soundType >> xPos >> yPos);
            SoundEffectsManager::getSingleton().playInterfaceSound(soundType,
                                                                   xPos, yPos);
            break;
        }

        case ServerNotificationType::notifyCreatureInfo:
        {
            std::string name;
            std::string infos;
            OD_ASSERT_TRUE(packetReceived >> name >> infos);
            Creature* creature = gameMap->getCreature(name);
            OD_ASSERT_TRUE_MSG(creature != nullptr, "name=" + name);
            if(creature == nullptr)
                break;

            creature->updateStatsWindow(infos);
            break;
        }

        case ServerNotificationType::refreshCreatureVisDebug:
        {
            std::string name;
            bool isDebugVisibleTilesActive;
            OD_ASSERT_TRUE(packetReceived >> name >> isDebugVisibleTilesActive);
            Creature* creature = gameMap->getCreature(name);
            OD_ASSERT_TRUE_MSG(creature != nullptr, "name=" + name);
            if(creature == nullptr)
                break;

            if(!isDebugVisibleTilesActive)
            {
                creature->destroyVisualDebugEntities();
                break;
            }

            uint32_t nbTiles;
            OD_ASSERT_TRUE(packetReceived >> nbTiles);
            std::vector<Tile*> tiles;
            while(nbTiles > 0)
            {
                --nbTiles;
                Tile* tile = gameMap->tileFromPacket(packetReceived);
                if(tile == nullptr)
                    continue;

                tiles.push_back(tile);
            }
            creature->refreshVisualDebugEntities(tiles);
            break;
        }

        case ServerNotificationType::refreshSeatVisDebug:
        {
            int seatId;
            bool isDebugVisibleTilesActive;
            OD_ASSERT_TRUE(packetReceived >> seatId >> isDebugVisibleTilesActive);
            Seat* seat = gameMap->getSeatById(seatId);
            OD_ASSERT_TRUE_MSG(seat != nullptr, "seatId=" + Ogre::StringConverter::toString(seatId));
            if(seat == nullptr)
                break;

            if(!isDebugVisibleTilesActive)
            {
                seat->stopVisualDebugEntities();
                break;
            }

            uint32_t nbTiles;
            OD_ASSERT_TRUE(packetReceived >> nbTiles);
            std::vector<Tile*> tiles;
            while(nbTiles > 0)
            {
                --nbTiles;
                Tile* tile = gameMap->tileFromPacket(packetReceived);
                if(tile == nullptr)
                    continue;

                tiles.push_back(tile);
            }
            seat->refreshVisualDebugEntities(tiles);
            break;
        }

        case ServerNotificationType::refreshVisibleTiles:
        {
            uint32_t nbTiles;
            // Tiles we gained vision
            OD_ASSERT_TRUE(packetReceived >> nbTiles);
            while(nbTiles > 0)
            {
                --nbTiles;
                Tile* tile = gameMap->tileFromPacket(packetReceived);
                if(tile == nullptr)
                    continue;

                tile->setLocalPlayerHasVision(true);
                tile->refreshMesh();
            }
            // Tiles we lost vision
            OD_ASSERT_TRUE(packetReceived >> nbTiles);
            while(nbTiles > 0)
            {
                --nbTiles;
                Tile* tile = gameMap->tileFromPacket(packetReceived);
                if(tile == nullptr)
                    continue;

                tile->setLocalPlayerHasVision(false);
                tile->refreshMesh();
            }
            break;
        }

        case ServerNotificationType::playCreatureSound:
        {
            std::string name;
            CreatureSoundType soundType;
            Ogre::Vector3 position;
            OD_ASSERT_TRUE(packetReceived >> name >> soundType >> position);
            CreatureSound* creatureSound = SoundEffectsManager::getSingleton().getCreatureClassSounds(name);
            OD_ASSERT_TRUE_MSG(creatureSound != nullptr, "name=" + name);
            if(creatureSound == nullptr)
                break;

            creatureSound->play(soundType, position.x, position.y, position.z);
            break;
        }

        case ServerNotificationType::refreshTiles:
        {
            uint32_t nbTiles;
            OD_ASSERT_TRUE(packetReceived >> nbTiles);
            std::vector<Tile*> tiles;
            while(nbTiles > 0)
            {
                --nbTiles;
                Tile* gameTile = gameMap->tileFromPacket(packetReceived);
                if(gameTile == nullptr)
                    continue;

                gameTile->updateFromPacket(packetReceived);
                tiles.push_back(gameTile);
            }
            gameMap->refreshBorderingTilesOf(tiles);
            break;
        }

        case ServerNotificationType::markTiles:
        {
            bool digSet;
            uint32_t nbTiles;
            OD_ASSERT_TRUE(packetReceived >> digSet >> nbTiles);

            SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::DIGSELECT);

            Player* player = getPlayer();
            while(nbTiles > 0)
            {
                --nbTiles;
                Tile* tile = gameMap->tileFromPacket(packetReceived);
                if(tile == nullptr)
                    continue;

                tile->setMarkedForDigging(digSet, player);
                tile->refreshMesh();
            }
            break;
        }

        case ServerNotificationType::carryEntity:
        {
            std::string carrierName;
            GameEntityType entityType;
            std::string carriedName;
            OD_ASSERT_TRUE(packetReceived >> carrierName >> entityType >> carriedName);
            Creature* carrier = gameMap->getCreature(carrierName);
            OD_ASSERT_TRUE_MSG(carrier != nullptr, "carrierName=" + carrierName);
            if(carrier == nullptr)
                break;

            GameEntity* carried = gameMap->getEntityFromTypeAndName(entityType, carriedName);
            OD_ASSERT_TRUE_MSG(carried != nullptr, "entityType=" + Ogre::StringConverter::toString(static_cast<int32_t>(entityType)) + ", carriedName=" + carriedName);
            if(carried == nullptr)
                break;

            RenderManager::getSingleton().rrCarryEntity(carrier, carried);
            break;
        }

        case ServerNotificationType::releaseCarriedEntity:
        {
            std::string carrierName;
            GameEntityType entityType;
            std::string carriedName;
            Ogre::Vector3 pos;
            OD_ASSERT_TRUE(packetReceived >> carrierName >> entityType >> carriedName >> pos);
            Creature* carrier = gameMap->getCreature(carrierName);
            OD_ASSERT_TRUE_MSG(carrier != nullptr, "carrierName=" + carrierName);
            if(carrier == nullptr)
                break;

            GameEntity* carried = gameMap->getEntityFromTypeAndName(entityType, carriedName);
            OD_ASSERT_TRUE_MSG(carried != nullptr, "entityType=" + Ogre::StringConverter::toString(static_cast<int32_t>(entityType)) + ", carriedName=" + carriedName);
            if(carried == nullptr)
                break;

            RenderManager::getSingleton().rrReleaseCarriedEntity(carrier, carried);
            carried->setPosition(pos, false);
            break;
        }

        case ServerNotificationType::researchTree:
        {
            uint32_t nbItems;
            OD_ASSERT_TRUE(packetReceived >> nbItems);
            std::vector<ResearchType> researches;
            while(nbItems > 0)
            {
                nbItems--;
                ResearchType research;
                OD_ASSERT_TRUE(packetReceived >> research);
                researches.push_back(research);
            }

            getPlayer()->getSeat()->setResearchTree(researches);
            break;
        }

        case ServerNotificationType::researchStarted:
        {
            ResearchType resType = ResearchType::nullResearchType;
            OD_ASSERT_TRUE(packetReceived >> resType);

            getPlayer()->getSeat()->setNextResearch(resType);
            break;
        }

        case ServerNotificationType::researchWaiting:
        {
            ResearchType resType = ResearchType::nullResearchType;
            OD_ASSERT_TRUE(packetReceived >> resType);

            getPlayer()->getSeat()->addResearchWaiting(resType);
            break;
        }

        case ServerNotificationType::researchesDone:
        {
            uint32_t nbItems;
            OD_ASSERT_TRUE(packetReceived >> nbItems);
            std::vector<ResearchType> researches;
            while(nbItems > 0)
            {
                nbItems--;
                ResearchType research;
                OD_ASSERT_TRUE(packetReceived >> research);
                researches.push_back(research);
            }

            getPlayer()->getSeat()->setResearchesDone(researches);
            break;
        }

        case ServerNotificationType::playerEvents:
        {
            uint32_t nbItems;
            OD_ASSERT_TRUE(packetReceived >> nbItems);
            std::vector<PlayerEvent*> events;
            while(nbItems > 0)
            {
                nbItems--;
                PlayerEvent* event = new PlayerEvent;
                event->importFromPacket(gameMap, packetReceived);
                events.push_back(event);
            }

            getPlayer()->updateEvents(events);
            break;
        }

        default:
        {
            logManager.logMessage("ERROR:  Unknown server command:"
                + Helper::toString(static_cast<int>(serverCommand)));
            break;
        }
    }

    return true;
}

void ODClient::processClientNotifications()
{
    while (isConnected())
    {
        // Wait until a message is place in the queue

        // Test whether there is something left to deal with
        if (mClientNotificationQueue.empty())
            break;

        ClientNotification* event = mClientNotificationQueue.front();
        mClientNotificationQueue.pop_front();

        if (!event)
            continue;

        switch (event->mType)
        {
            // If there are specific things to do before sending, it can be done here
            default:
                sendToServer(event->mPacket);
                break;
        }
        delete event;
    }
}

void ODClient::addChatMessage(ChatMessage* chat)
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    if (frameListener->getModeManager()->getCurrentModeType() == AbstractModeManager::GAME)
    {
        GameMode* gm = static_cast<GameMode*>(frameListener->getModeManager()->getCurrentMode());
        gm->receiveChat(chat);
    }
    // Note: Later, we can handle other modes here.
}

void ODClient::addEventMessage(EventMessage* event)
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    if (frameListener->getModeManager()->getCurrentModeType() == AbstractModeManager::GAME)
    {
        GameMode* gm = static_cast<GameMode*>(frameListener->getModeManager()->getCurrentMode());
        gm->receiveEventShortNotice(event);
    }
    // Note: Later, we can handle other modes here.
}

void ODClient::refreshMainUI(const std::string& goalsString)
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    if (frameListener->getModeManager()->getCurrentModeType() == AbstractModeManager::GAME)
    {
        GameMode* gm = static_cast<GameMode*>(frameListener->getModeManager()->getCurrentMode());
        gm->refreshPlayerGoals(goalsString);
        gm->refreshMainUI();
    }
    // Note: Later, we can handle other modes here if necessary.
}

void ODClient::sendToServer(ODPacket& packetToSend)
{
    send(packetToSend);
}

bool ODClient::connect(const std::string& host, const int port)
{
    LogManager& logManager = LogManager::getSingleton();
    mIsPlayerConfig = false;
    // Start the server socket listener as well as the server socket thread
    if (ODClient::getSingleton().isConnected())
    {
        logManager.logMessage("Couldn't try to connect: The client is already connected");
        return false;
    }

    if(!ODSocketClient::connect(host, port))
        return false;

    // Send a hello request to start the conversation with the server
    ODPacket packSend;
    packSend << ClientNotificationType::hello
        << std::string("OpenDungeons V ") + ODApplication::VERSION;
    sendToServer(packSend);

    return true;
}

bool ODClient::replay(const std::string& filename)
{
    LogManager& logManager = LogManager::getSingleton();
    mIsPlayerConfig = false;
    // Start the server socket listener as well as the server socket thread
    if (ODClient::getSingleton().isConnected())
    {
        logManager.logMessage("Couldn't try to launch replay: The client is already connected");
        return false;
    }

    if(!ODSocketClient::replay(filename))
        return false;

    return true;
}

void ODClient::queueClientNotification(ClientNotification* n)
{
    mClientNotificationQueue.push_back(n);
}

void ODClient::disconnect(bool keepReplay)
{
    ODSocketClient::disconnect(keepReplay);
    while(!mClientNotificationQueue.empty())
    {
        delete mClientNotificationQueue.front();
        mClientNotificationQueue.pop_front();
    }

    mIsPlayerConfig = false;
}

void ODClient::notifyExit()
{
    disconnect();
}
