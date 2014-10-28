/*!
 *  Copyright (C) 2011-2014  OpenDungeons Team
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
#include "entities/MapLight.h"
#include "entities/Creature.h"
#include "ODApplication.h"
#include "rooms/RoomTreasury.h"
#include "entities/TreasuryObject.h"
#include "entities/RenderedMovableEntity.h"
#include "utils/LogManager.h"
#include "modes/ModeManager.h"
#include "sound/MusicPlayer.h"
#include "camera/CameraManager.h"

#include <string>

template<> ODClient* Ogre::Singleton<ODClient>::msSingleton = 0;

ODClient::ODClient() :
    ODSocketClient()
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
        // Place a chat message in the queue to inform
        // the user about the disconnect
        frameListener->addChatMessage(new ChatMessage(ODServer::SERVER_INFORMATION,
            "Disconnected from server."));
        // TODO : try to reconnect to the server
        return false;
    }

    ServerNotification::ServerNotificationType serverCommand;
    OD_ASSERT_TRUE(packetReceived >> serverCommand);

    switch(serverCommand)
    {
        case ServerNotification::loadLevel:
        {
            std::string levelFilename;
            OD_ASSERT_TRUE(packetReceived >> levelFilename);
            // Read in the map. The map loading should happen here and not in the server thread to
            // make sure it is valid before launching the server.
            RenderManager::getSingletonPtr()->processRenderRequests();
            ODFrameListener::getSingleton().getClientGameMap()->processDeletionQueues();
            if (!gameMap->loadLevel(levelFilename))
            {
                // We disconnect as we don't have the map.
                logManager.logMessage("Disconnection. The level file can't be loaded: " + levelFilename);
                disconnect();
                return false;
            }

            // We set local player
            setPlayer(gameMap->getLocalPlayer());

            // Fill seats with either player, AIs or nothing depending on the given faction.
            uint32_t i = 0;
            uint32_t nbAiSeat = 0;
            uint32_t nbPlayerSeat = 0;
            while (i < gameMap->numEmptySeats())
            {
                Seat* seat = gameMap->getEmptySeat(i);

                if (seat->mFaction == "Player")
                {
                    ++nbPlayerSeat;
                }
                else if (seat->mFaction == "KeeperAI")
                {
                    ++nbAiSeat;
                }
                ++i;
            }

            logManager.logMessage("Map has: " + Ogre::StringConverter::toString(nbPlayerSeat) + " Human players");
            logManager.logMessage("Map has: " + Ogre::StringConverter::toString(nbAiSeat) + " AI players");

            // If no player seat, the game cannot be launched
            if (nbPlayerSeat == 0)
            {
                logManager.logMessage("Disconnection. There is no available player seat in level: " + levelFilename);
                disconnect();
                return false;
            }

            mLevelFilename = levelFilename;

            ODPacket packSend;
            packSend << ClientNotification::levelOK;
            send(packSend);
            break;
        }

        case ServerNotification::pickNick:
        {
            ODPacket packSend;
            packSend << ClientNotification::setNick << gameMap->getLocalPlayer()->getNick();
            send(packSend);
            break;
        }

        case ServerNotification::yourSeat:
        {
            int seatId;
            OD_ASSERT_TRUE(packetReceived >> seatId);
            Seat *tempSeat = gameMap->popEmptySeat(seatId);
            OD_ASSERT_TRUE_MSG(tempSeat != NULL, "seatId=" + Ogre::StringConverter::toString(seatId));
            gameMap->getLocalPlayer()->setSeat(tempSeat);
            // Move camera to starting position
            Ogre::Real startX = (Ogre::Real)(tempSeat->mStartingX);
            Ogre::Real startY = (Ogre::Real)(tempSeat->mStartingY);
            // We make the temple appear in the center of the game view
            startY = (Ogre::Real)(startY - 7.0);
            // Bound check
            if (startY <= 0.0)
            startY = 0.0;
            frameListener->setCameraPosition(Ogre::Vector3(startX, startY, MAX_CAMERA_Z));
            break;
        }

        case ServerNotification::clientAccepted:
        {
            ODServer::ServerMode serverMode;
            OD_ASSERT_TRUE(packetReceived >> serverMode);

            // Now that the we have received all needed information, we can launch the requested mode
            switch(serverMode)
            {
                case ODServer::ServerMode::ModeGame:
                    frameListener->getModeManager()->requestGameMode(true);
                    break;
                case ODServer::ServerMode::ModeEditor:
                    frameListener->getModeManager()->requestEditorMode(true);
                    break;
                default:
                    OD_ASSERT_TRUE_MSG(false,"Unknown server mode=" + Ogre::StringConverter::toString(static_cast<int32_t>(serverMode)));
            }

            break;
        }

        case ServerNotification::addPlayer:
        {
            Player *tempPlayer = new Player();
            std::string nick;
            int seatId;
            OD_ASSERT_TRUE(packetReceived >> nick >> seatId);
            tempPlayer->setNick(nick);
            Seat* seat = gameMap->popEmptySeat(seatId);
            OD_ASSERT_TRUE_MSG(seat != NULL, "seatId=" + Ogre::StringConverter::toString(seatId));
            gameMap->addPlayer(tempPlayer, seat);
            frameListener->addChatMessage(new ChatMessage(ODServer::SERVER_INFORMATION,
                "New player connected:" + tempPlayer->getNick()));
            break;
        }

        case ServerNotification::chat:
        {
            std::string chatNick;
            std::string chatMsg;
            OD_ASSERT_TRUE(packetReceived >> chatNick >> chatMsg);
            ChatMessage *newMessage = new ChatMessage(chatNick, chatMsg);
            frameListener->addChatMessage(newMessage);
            break;
        }

        case ServerNotification::chatServer:
        {
            std::string chatMsg;
            OD_ASSERT_TRUE(packetReceived >> chatMsg);
            ChatMessage *newMessage = new ChatMessage(ODServer::SERVER_INFORMATION,
                chatMsg);
            frameListener->addChatMessage(newMessage);
            break;
        }

        case ServerNotification::newMap:
        {
            gameMap->clearAll();
            break;
        }

        case ServerNotification::turnsPerSecond:
        {
            OD_ASSERT_TRUE(packetReceived >> ODApplication::turnsPerSecond);
            break;
        }

        case ServerNotification::addTile:
        {
            Tile* newTile = new Tile(gameMap);
            OD_ASSERT_TRUE(packetReceived >> newTile);
            gameMap->addTile(newTile);
            newTile->setFullness(newTile->getFullness());

            // Loop over the tile's neighbors to force them to recheck
            // their mesh to see if they can use an optimized one
            std::vector<Tile*> neighbors = newTile->getAllNeighbors();
            for (unsigned int i = 0; i < neighbors.size(); ++i)
            {
                neighbors[i]->setFullness(neighbors[i]->getFullness());
            }
            break;
        }

        case ServerNotification::addMapLight:
        {
            MapLight *newMapLight = new MapLight(gameMap, false);
            OD_ASSERT_TRUE(packetReceived >> newMapLight);
            gameMap->addMapLight(newMapLight);
            newMapLight->createOgreEntity();
            break;
        }

        case ServerNotification::removeMapLight:
        {
            std::string nameMapLight;
            OD_ASSERT_TRUE(packetReceived >> nameMapLight);
            MapLight *tempMapLight = gameMap->getMapLight(nameMapLight);
            OD_ASSERT_TRUE_MSG(tempMapLight != NULL, "nameMapLight=" + nameMapLight);
            gameMap->removeMapLight(tempMapLight);
            tempMapLight->deleteYourself();
            break;
        }

        case ServerNotification::addClass:
        {
            CreatureDefinition *tempClass = new CreatureDefinition;
            OD_ASSERT_TRUE(packetReceived >> tempClass);
            gameMap->addClassDescription(tempClass);
            break;
        }

        case ServerNotification::addCreature:
        {
            Creature *newCreature = Creature::getCreatureFromPacket(gameMap, packetReceived);
            gameMap->addCreature(newCreature);
            newCreature->createMesh();
            break;
        }

        case ServerNotification::removeCreature:
        {
            std::string creatureName;
            OD_ASSERT_TRUE(packetReceived >> creatureName);
            Creature* creature = gameMap->getCreature(creatureName);
            OD_ASSERT_TRUE_MSG(creature != NULL, "creatureName=" + creatureName);
            if(creature != NULL)
            {
                gameMap->removeCreature(creature);
                creature->deleteYourself();
            }
            break;
        }

        case ServerNotification::turnStarted:
        {
            int64_t turnNum;
            OD_ASSERT_TRUE(packetReceived >> turnNum);
            logManager.logMessage("Client received turnStarted="
                + Ogre::StringConverter::toString((int32_t)turnNum));
            gameMap->setTurnNumber(turnNum);

            // We acknowledge the new turn to the server so that he knows we are
            // ready for next one
            ODPacket packSend;
            packSend << ClientNotification::ackNewTurn << turnNum;
            send(packSend);
            break;
        }

        case ServerNotification::animatedObjectAddDestination:
        {
            std::string objName;
            Ogre::Vector3 vect;
            OD_ASSERT_TRUE(packetReceived >> objName >> vect);
            MovableGameEntity *tempAnimatedObject = gameMap->getAnimatedObject(objName);
            OD_ASSERT_TRUE_MSG(tempAnimatedObject != NULL, "objName=" + objName);
            if (tempAnimatedObject != NULL)
                tempAnimatedObject->addDestination(vect.x, vect.y, vect.z);

            break;
        }

        case ServerNotification::animatedObjectClearDestinations:
        {
            std::string objName;
            OD_ASSERT_TRUE(packetReceived >> objName);
            MovableGameEntity *tempAnimatedObject = gameMap->getAnimatedObject(objName);
            OD_ASSERT_TRUE_MSG(tempAnimatedObject != NULL, "objName=" + objName);
            if (tempAnimatedObject != NULL)
                tempAnimatedObject->clearDestinations();

            break;
        }

        case ServerNotification::entityPickedUp:
        {
            ODServer::ServerMode serverMode;
            int seatId;
            GameEntity::ObjectType entityType;
            std::string entityName;
            GameEntity* entity = nullptr;
            OD_ASSERT_TRUE(packetReceived >> serverMode >> seatId >> entityType >> entityName);
            Player *tempPlayer = gameMap->getPlayerBySeatId(seatId);
            OD_ASSERT_TRUE_MSG(tempPlayer != nullptr, "seatId=" + Ogre::StringConverter::toString(seatId));
            if(tempPlayer == nullptr)
                break;

            switch(entityType)
            {
                case GameEntity::ObjectType::creature:
                {
                    entity = gameMap->getCreature(entityName);
                    break;
                }
                case GameEntity::ObjectType::renderedMovableEntity:
                {
                    entity = gameMap->getRenderedMovableEntity(entityName);
                    break;
                }
                default:
                    // No need to display an error as it will be displayed in the following assert
                    break;
            }
            OD_ASSERT_TRUE_MSG(entity != nullptr, "entityType=" + Ogre::StringConverter::toString(static_cast<int32_t>(entityType)) + ", entityName=" + entityName);
            if(entity == nullptr)
                break;

            tempPlayer->pickUpEntity(entity, serverMode == ODServer::ServerMode::ModeEditor);
            break;
        }

        case ServerNotification::entityDropped:
        {
            int seatId;
            Tile tmpTile(gameMap);
            OD_ASSERT_TRUE(packetReceived >> seatId >> &tmpTile);
            Player *tempPlayer = gameMap->getPlayerBySeatId(seatId);
            OD_ASSERT_TRUE_MSG(tempPlayer != nullptr, "seatId=" + Ogre::StringConverter::toString(seatId));
            Tile* tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            OD_ASSERT_TRUE_MSG(tile != nullptr, "tile=" + Tile::displayAsString(&tmpTile));
            if (tempPlayer != nullptr && tile != nullptr)
            {
                OD_ASSERT_TRUE(tempPlayer->dropHand(tile) != nullptr);
            }
            break;
        }

        case ServerNotification::setObjectAnimationState:
        {
            std::string objName;
            std::string animState;
            bool loop;
            bool shouldSetWalkDirection;
            OD_ASSERT_TRUE(packetReceived >> objName >> animState
                >> loop >> shouldSetWalkDirection);
            MovableGameEntity *obj = gameMap->getAnimatedObject(objName);
            OD_ASSERT_TRUE_MSG(obj != NULL, "objName=" + objName + ", state=" + animState);
            if (obj != NULL)
            {
                if(shouldSetWalkDirection)
                {
                    Ogre::Vector3 walkDirection;
                    OD_ASSERT_TRUE(packetReceived >> walkDirection);
                    obj->setWalkDirection(walkDirection);
                }
                obj->setAnimationState(animState, loop);
            }
            else
            {
                logManager.logMessage("ERROR: Server told us to change animations for a nonexistent object="
                    + objName);
            }
            break;
        }

        case ServerNotification::setMoveSpeed:
        {
            std::string objName;
            double moveSpeed;
            OD_ASSERT_TRUE(packetReceived >> objName >> moveSpeed);
            MovableGameEntity *obj = gameMap->getAnimatedObject(objName);
            OD_ASSERT_TRUE_MSG(obj != NULL, "objName=" + objName + ", moveSpeed=" + Ogre::StringConverter::toString(moveSpeed));
            if (obj == nullptr)
                break;

            obj->setMoveSpeed(moveSpeed);
            break;
        }

        case ServerNotification::tileFullnessChange:
        {
            Tile tmpTile(gameMap);

            OD_ASSERT_TRUE(packetReceived >> &tmpTile);

            Tile *tempTile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            OD_ASSERT_TRUE_MSG(tempTile != NULL, "tile=" + Tile::displayAsString(&tmpTile));
            if (tempTile != NULL)
            {
                tempTile->setFullness(tmpTile.getFullness());
                std::vector<Tile*> neighbors = tempTile->getAllNeighbors();
                for (std::vector<Tile*>::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
                {
                    Tile* neighbor = *it;
                    neighbor->setFullness(neighbor->getFullness());
                }
            }
            break;
        }

        case ServerNotification::tileClaimed:
        {
            Tile tmpTile(gameMap);
            OD_ASSERT_TRUE(packetReceived >> &tmpTile);
            Tile *tempTile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            OD_ASSERT_TRUE_MSG(tempTile != NULL, "tile=" + Tile::displayAsString(&tmpTile));
            if (tempTile != NULL)
            {
                Seat* seat = tmpTile.getSeat();
                OD_ASSERT_TRUE_MSG(seat != NULL, "tile=" + Tile::displayAsString(&tmpTile));
                tempTile->claimTile(seat);
                SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::CLAIMED, tmpTile.getX(), tmpTile.getY());
            }
            break;
        }

        case ServerNotification::refreshPlayerSeat:
        {
            Seat tmpSeat;
            std::string goalsString;
            OD_ASSERT_TRUE(packetReceived >> &tmpSeat >> goalsString);
            getPlayer()->getSeat()->refreshFromSeat(&tmpSeat);
            frameListener->refreshPlayerDisplay(goalsString);
            break;
        }

        case ServerNotification::markTiles:
        {
            bool isDigSet;
            int nbTiles;
            OD_ASSERT_TRUE(packetReceived >> isDigSet >> nbTiles);
            std::vector<Tile*> tiles;
            for(int i = 0; i < nbTiles; i++)
            {
                Tile tmpTile(gameMap);
                OD_ASSERT_TRUE(packetReceived >> &tmpTile);
                Tile* gameTile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
                OD_ASSERT_TRUE_MSG(gameTile != NULL, "tile=" + Tile::displayAsString(&tmpTile));
                if(gameTile != NULL)
                    tiles.push_back(gameTile);
            }
            gameMap->markTilesForPlayer(tiles, isDigSet, gameMap->getLocalPlayer());
            SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::DIGSELECT);
            break;
        }

        case ServerNotification::buildRoom:
        {
            Room* room = Room::getRoomFromPacket(gameMap, packetReceived);
            OD_ASSERT_TRUE(room != nullptr);
            std::vector<Tile*> tiles = room->getCoveredTiles();
            for(std::vector<Tile*>::const_iterator it = tiles.begin(); it != tiles.end(); ++it)
            {
                Tile* tile = *it;
                tile->setType(Tile::TileType::claimed);
                tile->setSeat(room->getSeat());
                tile->setFullness(0.0);
            }
            gameMap->addRoom(room);
            room->checkForRoomAbsorbtion();
            room->createMesh();
            room->updateActiveSpots();
            gameMap->refreshBorderingTilesOf(tiles);

            if(gameMap->getLocalPlayer()->getSeat() == room->getSeat())
            {
                SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUILDROOM);
            }
            break;
        }

        case ServerNotification::removeRoomTile:
        {
            std::string roomName;
            Tile tmpTile(gameMap);
            OD_ASSERT_TRUE(packetReceived >> roomName>> &tmpTile);
            Room* room = gameMap->getRoomByName(roomName);
            OD_ASSERT_TRUE_MSG(room != nullptr, "roomName=" + roomName);
            Tile* tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            OD_ASSERT_TRUE_MSG(tile != nullptr, "tile=" + Tile::displayAsString(&tmpTile));
            if((room != nullptr) && (tile != nullptr))
            {
                room->removeCoveredTile(tile);
                // If no more tiles, the room is removed
                if (room->numCoveredTiles() <= 0)
                {
                    gameMap->removeRoom(room);
                    room->deleteYourself();
                    break;
                }
                room->updateActiveSpots();
            }

            break;
        }

        case ServerNotification::removeTrapTile:
        {
            std::string trapName;
            Tile tmpTile(gameMap);
            OD_ASSERT_TRUE(packetReceived >> trapName>> &tmpTile);
            Trap* trap = gameMap->getTrapByName(trapName);
            OD_ASSERT_TRUE_MSG(trap != nullptr, "trapName=" + trapName);
            Tile* tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            OD_ASSERT_TRUE_MSG(tile != nullptr, "tile=" + Tile::displayAsString(&tmpTile));
            if((trap != nullptr) && (tile != nullptr))
            {
                trap->removeCoveredTile(tile);
                // If no more tiles, the room is removed
                if (trap->numCoveredTiles() <= 0)
                {
                    gameMap->removeTrap(trap);
                    trap->deleteYourself();
                }
            }

            break;
        }

        case ServerNotification::buildTrap:
        {
            Trap* trap = Trap::getTrapFromPacket(gameMap, packetReceived);
            OD_ASSERT_TRUE(trap != nullptr);
            std::vector<Tile*> tiles = trap->getCoveredTiles();
            for(std::vector<Tile*>::const_iterator it = tiles.begin(); it != tiles.end(); ++it)
            {
                Tile* tile = *it;
                tile->setType(Tile::TileType::claimed);
                tile->setSeat(trap->getSeat());
                tile->setFullness(0.0);
            }
            gameMap->addTrap(trap);
            trap->createMesh();
            trap->updateActiveSpots();

            if(gameMap->getLocalPlayer()->getSeat() == trap->getSeat())
            {
                SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUILDTRAP);
            }
            break;
        }

        case ServerNotification::creatureRefresh:
        {
            Creature tmpCreature(gameMap);
            tmpCreature.importFromPacket(packetReceived);
            Creature* creature = gameMap->getCreature(tmpCreature.getName());
            OD_ASSERT_TRUE_MSG(creature != NULL, "name=" + tmpCreature.getName());
            if(creature != NULL)
                creature->refreshFromCreature(&tmpCreature);
            break;
        }

        case ServerNotification::playerFighting:
        {
            std::string fightMusic = gameMap->getLevelFightMusicFile();
            if (fightMusic.empty())
                break;
            MusicPlayer::getSingleton().play(fightMusic);
            break;
        }

        case ServerNotification::playerNoMoreFighting:
        {
            MusicPlayer::getSingleton().play(gameMap->getLevelMusicFile());
            break;
        }

        case ServerNotification::addRenderedMovableEntity:
        {
            RenderedMovableEntity* tempRenderedMovableEntity = RenderedMovableEntity::getRenderedMovableEntityFromPacket(gameMap, packetReceived);
            OD_ASSERT_TRUE(tempRenderedMovableEntity != nullptr);
            gameMap->addRenderedMovableEntity(tempRenderedMovableEntity);
            tempRenderedMovableEntity->createMesh();
            break;
        }

        case ServerNotification::removeRenderedMovableEntity:
        {
            std::string name;
            OD_ASSERT_TRUE(packetReceived >> name);
            RenderedMovableEntity* tempRenderedMovableEntity = gameMap->getRenderedMovableEntity(name);
            OD_ASSERT_TRUE_MSG(tempRenderedMovableEntity != NULL, "name=" + name);
            gameMap->removeRenderedMovableEntity(tempRenderedMovableEntity);
            tempRenderedMovableEntity->deleteYourself();
            break;
        }

        case ServerNotification::playSpatialSound:
        {
            SoundEffectsManager::InterfaceSound soundType;
            int xPos;
            int yPos;
            OD_ASSERT_TRUE(packetReceived >> soundType >> xPos >> yPos);
            SoundEffectsManager::getSingleton().playInterfaceSound(soundType,
                                                                   xPos, yPos);
            break;
        }

        case ServerNotification::notifyCreatureInfo:
        {
            std::string name;
            std::string infos;
            OD_ASSERT_TRUE(packetReceived >> name >> infos);
            Creature* creature = gameMap->getCreature(name);
            OD_ASSERT_TRUE_MSG(creature != NULL, "name=" + name);
            if(creature != NULL)
                creature->updateStatsWindow(infos);
            break;
        }

        case ServerNotification::playCreatureSound:
        {
            std::string name;
            CreatureSound::SoundType soundType;
            OD_ASSERT_TRUE(packetReceived >> name >> soundType);
            Creature* creature = gameMap->getCreature(name);
            OD_ASSERT_TRUE_MSG(creature != NULL, "name=" + name);
            if(creature != NULL)
                creature->playSound(soundType);
            break;
        }

        case ServerNotification::refreshTiles:
        {
            int nbTiles;
            OD_ASSERT_TRUE(packetReceived >> nbTiles);
            std::vector<Tile*> tiles;
            for(int i = 0; i < nbTiles; i++)
            {
                Tile tmpTile(gameMap);
                OD_ASSERT_TRUE(packetReceived >> &tmpTile);
                Tile* gameTile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
                OD_ASSERT_TRUE(gameTile != NULL);
                if(gameTile == NULL)
                    continue;
                gameTile->refreshFromTile(tmpTile);
                tiles.push_back(gameTile);
            }
            gameMap->refreshBorderingTilesOf(tiles);
            break;
        }

        default:
        {
            logManager.logMessage("ERROR:  Unknown server command:"
                + Ogre::StringConverter::toString(serverCommand));
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
    }
}

void ODClient::sendToServer(ODPacket& packetToSend)
{
    send(packetToSend);
}

bool ODClient::connect(const std::string& host, const int port)
{
    LogManager& logManager = LogManager::getSingleton();
    // Start the server socket listener as well as the server socket thread
    if (ODClient::getSingleton().isConnected())
    {
        logManager.logMessage("Couldn't try to connect: The client is already connected");
        return false;
    }

    // Start by loading map
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    if (gameMap == NULL)
        return false;

    if(!ODSocketClient::connect(host, port))
        return false;

    // Send a hello request to start the conversation with the server
    ODPacket packSend;
    packSend << ClientNotification::hello
        << std::string("OpenDungeons V ") + ODApplication::VERSION;
    sendToServer(packSend);

    return true;
}

void ODClient::queueClientNotification(ClientNotification* n)
{
    mClientNotificationQueue.push_back(n);
}

void ODClient::disconnect()
{
    ODSocketClient::disconnect();
    while(!mClientNotificationQueue.empty())
    {
        delete mClientNotificationQueue.front();
        mClientNotificationQueue.pop_front();
    }
}

void ODClient::notifyExit()
{
    disconnect();
}
