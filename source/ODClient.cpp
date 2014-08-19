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

#include "ODClient.h"
#include "ODServer.h"
#include "ODPacket.h"
#include "ServerNotification.h"
#include "ODFrameListener.h"
#include "RenderManager.h"
#include "ChatMessage.h"
#include "GameMap.h"
#include "Seat.h"
#include "Player.h"
#include "MapLight.h"
#include "Creature.h"
#include "Weapon.h"
#include "ODApplication.h"
#include "RoomQuarters.h"
#include "RoomTreasury.h"
#include "MissileObject.h"
#include "RoomObject.h"
#include "LogManager.h"

#include <string>

template<> ODClient* Ogre::Singleton<ODClient>::msSingleton = 0;

ODClient::ODClient() :
    ODSocketClient()
{
}

ODClient::~ODClient()
{
}


void ODClient::processClientSocketMessages()
{
    // If we receive message for a new turn, after processing every message,
    // we will refresh what is needed
    // We loop until no more data is available
    while(isConnected() && processOneClientSocketMessage());
}

bool ODClient::processOneClientSocketMessage()
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();

    GameMap* gameMap = frameListener->getGameMap();
    if (!gameMap)
        return false;

    if(!isDataAvailable())
        return false;

    // Get a reference to the LogManager
    LogManager& logMgr = LogManager::getSingleton();

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
        case ServerNotification::pickNick:
        {
            ODPacket packSend;
            packSend << ClientNotification::setNick << gameMap->getLocalPlayer()->getNick();
            send(packSend);
            break;
        }

        case ServerNotification::yourSeat:
        {
            int color;
            OD_ASSERT_TRUE(packetReceived >> color);
            Seat *tempSeat = gameMap->popEmptySeat(color);
            OD_ASSERT_TRUE_MSG(tempSeat != NULL, "color=" + Ogre::StringConverter::toString(color));
            gameMap->getLocalPlayer()->setSeat(tempSeat);
            break;
        }

        case ServerNotification::addPlayer:
        {
            Player *tempPlayer = new Player();
            std::string nick;
            int seatColor;
            OD_ASSERT_TRUE(packetReceived >> nick >> seatColor);
            tempPlayer->setNick(nick);
            Seat* seat = gameMap->popEmptySeat(seatColor);
            OD_ASSERT_TRUE_MSG(seat != NULL, "color=" + Ogre::StringConverter::toString(seatColor));
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
            std::string className;
            OD_ASSERT_TRUE(packetReceived >> className);
            CreatureDefinition *creatureClass = gameMap->getClassDescription(className);
            Creature *newCreature = new Creature(gameMap, creatureClass, false);
            OD_ASSERT_TRUE(packetReceived >> newCreature);
            gameMap->addCreature(newCreature);
            newCreature->createMesh();
            newCreature->getWeaponL()->createMesh();
            newCreature->getWeaponR()->createMesh();
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
            logMgr.logMessage("Client received turnStarted="
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
                tempAnimatedObject->addDestination(vect.x, vect.y);

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

        case ServerNotification::pickupCreature:
        {
            std::string creatureName;
            OD_ASSERT_TRUE(packetReceived >> creatureName);
            Player *tempPlayer = gameMap->getLocalPlayer();
            Creature *tempCreature = gameMap->getCreature(creatureName);
            OD_ASSERT_TRUE_MSG(tempCreature != NULL, "creatureName=" + creatureName);
            if (tempCreature != NULL)
            {
                tempPlayer->pickUpCreature(tempCreature);
            }
            break;
        }

        case ServerNotification::dropCreature:
        {
            Tile tmpTile(gameMap);
            OD_ASSERT_TRUE(packetReceived >> &tmpTile);
            Player *tempPlayer = gameMap->getLocalPlayer();
            Tile* tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            OD_ASSERT_TRUE_MSG(tile != NULL, "tmpTile=" + Ogre::StringConverter::toString(tmpTile.getX())
                + "," + Ogre::StringConverter::toString(tmpTile.getY()));
            if (tile != NULL)
            {
                tempPlayer->dropCreature(tile);
                SoundEffectsHelper::getSingleton().playInterfaceSound(SoundEffectsHelper::DROP);
            }
            break;
        }

        case ServerNotification::creaturePickedUp:
        {
            int playerColor;
            std::string creatureName;
            OD_ASSERT_TRUE(packetReceived >> playerColor >> creatureName);
            Player *tempPlayer = gameMap->getPlayerByColor(playerColor);
            OD_ASSERT_TRUE_MSG(tempPlayer != NULL, "playerColor=" + Ogre::StringConverter::toString(playerColor));
            Creature *tempCreature = gameMap->getCreature(creatureName);
            OD_ASSERT_TRUE_MSG(tempCreature != NULL, "creatureName=" + creatureName);
            if (tempPlayer != NULL && tempCreature != NULL)
            {
                tempPlayer->pickUpCreature(tempCreature);
            }
            break;
        }

        case ServerNotification::creatureDropped:
        {
            int playerColor;
            Tile tmpTile(gameMap);
            OD_ASSERT_TRUE(packetReceived >> playerColor >> &tmpTile);
            Player *tempPlayer = gameMap->getPlayerByColor(playerColor);
            OD_ASSERT_TRUE_MSG(tempPlayer != NULL, "playerColor=" + Ogre::StringConverter::toString(playerColor));
            Tile* tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            OD_ASSERT_TRUE_MSG(tile != NULL, "tile=" + Ogre::StringConverter::toString(tmpTile.getX())
                + "," + Ogre::StringConverter::toString(tmpTile.getY()));
            if (tempPlayer != NULL && tile != NULL)
            {
                tempPlayer->dropCreature(tile);
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
            OD_ASSERT_TRUE_MSG(obj != NULL, "objName=" + objName);
            if (obj != NULL)
            {
                if(shouldSetWalkDirection)
                {
                    Ogre::Vector3 walkDirection;
                    OD_ASSERT_TRUE(packetReceived >> walkDirection);
                    obj->setWalkDirection(walkDirection);
                }
                obj->setAnimationState(animState, false, loop);
            }
            else
            {
                logMgr.logMessage("ERROR: Server told us to change animations for a nonexistent object="
                    + objName);
            }
            break;
        }

        case ServerNotification::tileFullnessChange:
        {
            Tile tmpTile(gameMap);

            OD_ASSERT_TRUE(packetReceived >> &tmpTile);

            Tile *tempTile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            OD_ASSERT_TRUE_MSG(tempTile != NULL, "tile=" + Ogre::StringConverter::toString(tmpTile.getX())
                + "," + Ogre::StringConverter::toString(tmpTile.getY()));
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
            OD_ASSERT_TRUE_MSG(tempTile != NULL, "tile=" + Ogre::StringConverter::toString(tmpTile.getX())
                + "," + Ogre::StringConverter::toString(tmpTile.getY()));
            if (tempTile != NULL)
            {
                tempTile->claimTile(tmpTile.getColor());
            }
            break;
        }

        case ServerNotification::refreshPlayerSeat:
        {
            Seat tmpSeat;
            std::string goalsString;
            OD_ASSERT_TRUE(packetReceived >> &tmpSeat >> goalsString);
            getPlayer()->getSeat()->refreshFromSeat(&tmpSeat);
            ODFrameListener::getSingleton().refreshPlayerDisplay(goalsString);
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
                OD_ASSERT_TRUE_MSG(gameTile != NULL, "tile=" + Ogre::StringConverter::toString(tmpTile.getX())
                    + "," + Ogre::StringConverter::toString(tmpTile.getY()));
                if(gameTile != NULL)
                    tiles.push_back(gameTile);
            }
            gameMap->markTilesForPlayer(tiles, isDigSet, gameMap->getLocalPlayer());
            SoundEffectsHelper::getSingleton().playInterfaceSound(SoundEffectsHelper::DIGSELECT, false);
            break;
        }

        case ServerNotification::buildRoom:
        {
            int intType, color;
            Room::RoomType type;
            int nbTiles;
            OD_ASSERT_TRUE(packetReceived >> intType>> color >> nbTiles);
            type = static_cast<Room::RoomType>(intType);
            std::vector<Tile*> tiles;
            for(int i = 0; i < nbTiles; i++)
            {
                Tile tmpTile(gameMap);
                OD_ASSERT_TRUE(packetReceived >> &tmpTile);
                Tile* gameTile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
                OD_ASSERT_TRUE_MSG(gameTile != NULL, "tile=" + Ogre::StringConverter::toString(tmpTile.getX())
                    + "," + Ogre::StringConverter::toString(tmpTile.getY()));
                if(gameTile != NULL)
                    tiles.push_back(gameTile);
            }
            Player* player = gameMap->getPlayerByColor(color);
            OD_ASSERT_TRUE_MSG(player != NULL, "color=" + Ogre::StringConverter::toString(color));
            gameMap->buildRoomForPlayer(tiles, type, player);
            break;
        }

        case ServerNotification::removeRoomTile:
        {
            std::string roomName;
            Tile tmpTile(gameMap);
            OD_ASSERT_TRUE(packetReceived >> roomName>> &tmpTile);
            Room* room = gameMap->getRoomByName(roomName);
            OD_ASSERT_TRUE_MSG(room != NULL, "roomName=" + roomName);
            Tile* tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            OD_ASSERT_TRUE_MSG(tile != NULL, "tile=" + Ogre::StringConverter::toString(tmpTile.getX())
                + "," + Ogre::StringConverter::toString(tmpTile.getY()));
            if((room != NULL) && (tile != NULL))
            {
                room->removeCoveredTile(tile);
                room->updateActiveSpots();
            }

            break;
        }

        case ServerNotification::buildTrap:
        {
            int intType, color;
            Trap::TrapType type;
            int nbTiles;
            OD_ASSERT_TRUE(packetReceived >> intType >> color >> nbTiles);
            type = static_cast<Trap::TrapType>(intType);
            std::vector<Tile*> tiles;
            for(int i = 0; i < nbTiles; i++)
            {
                Tile tmpTile(gameMap);
                OD_ASSERT_TRUE(packetReceived >> &tmpTile);
                Tile* gameTile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
                tiles.push_back(gameTile);
            }
            Player* player = gameMap->getPlayerByColor(color);
            OD_ASSERT_TRUE_MSG(player != NULL, "color=" + Ogre::StringConverter::toString(color));
            gameMap->buildTrapForPlayer(tiles, type, player);
            break;
        }

        case ServerNotification::creatureRefresh:
        {
            Creature tmpCreature(gameMap);
            OD_ASSERT_TRUE(packetReceived >> &tmpCreature);
            Creature* creature = gameMap->getCreature(tmpCreature.getName());
            OD_ASSERT_TRUE_MSG(creature != NULL, "name=" + tmpCreature.getName());
            if(creature != NULL)
                creature->refreshFromCreature(&tmpCreature);
            break;
        }

        case ServerNotification::addMissileObject:
        {
            MissileObject* missile = new MissileObject(gameMap);
            OD_ASSERT_TRUE(packetReceived >> missile);
            gameMap->addMissileObject(missile);
            missile->createMesh();
            break;
        }

        case ServerNotification::removeMissileObject:
        {
            std::string name;
            OD_ASSERT_TRUE(packetReceived >> name);
            MissileObject* missile = gameMap->getMissileObject(name);
            OD_ASSERT_TRUE_MSG(missile != NULL, "name=" + name);
            if(missile != NULL)
            {
                gameMap->removeMissileObject(missile);
                missile->deleteYourself();
            }
            break;
        }

        case ServerNotification::createTreasuryIndicator:
        {
            int color;
            std::string roomName;
            Tile tmpTile(gameMap);
            std::string indicatorMeshName;
            OD_ASSERT_TRUE(packetReceived >> color >> roomName >> &tmpTile >> indicatorMeshName);
            Room* room = gameMap->getRoomByName(roomName);
            OD_ASSERT_TRUE_MSG(room != NULL, "name=" + roomName);
            Tile* tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            OD_ASSERT_TRUE_MSG(tile != NULL, "tile=" + Ogre::StringConverter::toString(tmpTile.getX())
                + "," + Ogre::StringConverter::toString(tmpTile.getY()));
            RoomTreasury* rt = static_cast<RoomTreasury*>(room);
            OD_ASSERT_TRUE_MSG(rt->getColor() == color, "roomColor=" + Ogre::StringConverter::toString(rt->getColor())
                + ",color=" + Ogre::StringConverter::toString(color));
            rt->createMeshesForTile(tile, indicatorMeshName);
            break;
        }

        case ServerNotification::destroyTreasuryIndicator:
        {
            int color;
            std::string roomName;
            Tile tmpTile(gameMap);
            std::string indicatorMeshName;
            OD_ASSERT_TRUE(packetReceived >> color >> roomName >> &tmpTile >> indicatorMeshName);
            Room* room = gameMap->getRoomByName(roomName);
            OD_ASSERT_TRUE_MSG(room != NULL, "name=" + roomName);
            Tile* tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            OD_ASSERT_TRUE_MSG(tile != NULL, "tile=" + Ogre::StringConverter::toString(tmpTile.getX())
                + "," + Ogre::StringConverter::toString(tmpTile.getY()));
            RoomTreasury* rt = static_cast<RoomTreasury*>(room);
            OD_ASSERT_TRUE_MSG(rt->getColor() == color, "roomColor=" + Ogre::StringConverter::toString(rt->getColor())
                + ",color=" + Ogre::StringConverter::toString(color));
            rt->destroyMeshesForTile(tile, indicatorMeshName);
            break;
        }

        case ServerNotification::addRoomObject:
        {
            std::string roomName;
            OD_ASSERT_TRUE(packetReceived >> roomName);
            Room* room = gameMap->getRoomByName(roomName);
            OD_ASSERT_TRUE_MSG(room != NULL, "name=" + roomName);
            Tile tmpTile(gameMap);
            OD_ASSERT_TRUE(packetReceived >> &tmpTile);
            Tile* tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            OD_ASSERT_TRUE_MSG(tile != NULL, "tile=" + Ogre::StringConverter::toString(tmpTile.getX())
                + "," + Ogre::StringConverter::toString(tmpTile.getY()));
            RoomObject* tempRoomObject = new RoomObject(gameMap, room);
            OD_ASSERT_TRUE(packetReceived >> tempRoomObject);
            room->addRoomObject(tile, tempRoomObject);
            tempRoomObject->createMesh();
            break;
        }

        case ServerNotification::removeRoomObject:
        {
            std::string roomName;
            Tile tmpTile(gameMap);
            OD_ASSERT_TRUE(packetReceived >> roomName >> &tmpTile);
            Room* room = gameMap->getRoomByName(roomName);
            OD_ASSERT_TRUE_MSG(room != NULL, "name=" + roomName);
            Tile *tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            OD_ASSERT_TRUE_MSG(tile != NULL, "tile=" + Ogre::StringConverter::toString(tmpTile.getX())
                + "," + Ogre::StringConverter::toString(tmpTile.getY()));
            RoomObject* tempRoomObject = room->getRoomObjectFromTile(tile);
            OD_ASSERT_TRUE_MSG(tempRoomObject != NULL, "roomName=" + roomName + ",tile="
                + Ogre::StringConverter::toString(tmpTile.getX())
                + "," + Ogre::StringConverter::toString(tmpTile.getY()));
            room->removeRoomObject(tempRoomObject);
            break;
        }

        case ServerNotification::removeAllRoomObjectFromRoom:
        {
            std::string roomName;
            Tile tmpTile(gameMap);
            OD_ASSERT_TRUE(packetReceived >> roomName);
            Room* room = gameMap->getRoomByName(roomName);
            OD_ASSERT_TRUE_MSG(room != NULL, "roomName=" + roomName);
            room->removeAllRoomObject();
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

        default:
        {
            logMgr.logMessage("ERROR:  Unknown server command:"
                + Ogre::StringConverter::toString(serverCommand));
            break;
        }
    }

    return true;
}

void ODClient::processClientNotifications()
{
    ODPacket packSend;
    Tile* tempTile = NULL;
    Creature* tempCreature = NULL;
    Player* tempPlayer = NULL;
    bool flag = false;

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
            case ClientNotification::askCreaturePickUp:
                sendToServer(event->packet);
                break;

            case ClientNotification::askCreatureDrop:
                sendToServer(event->packet);
                break;

            case ClientNotification::askMarkTile:
                sendToServer(event->packet);
                break;

            case ClientNotification::askBuildRoom:
                sendToServer(event->packet);
                break;

            case ClientNotification::askBuildTrap:
                sendToServer(event->packet);
                break;

            case ClientNotification::chat:
                sendToServer(event->packet);
                break;

            case ClientNotification::askCreatureInfos:
                sendToServer(event->packet);
                break;

            default:
                LogManager& logMgr = LogManager::getSingleton();
                ODApplication::displayErrorMessage("Unhandled ClientNotification type encountered:"
                    + Ogre::StringConverter::toString(event->mType));
                logMgr.logMessage("Unhandled ClientNotification type encountered:"
                    + Ogre::StringConverter::toString(event->mType));

                // This is forcing a core dump so I can debug why this happened
                // Enable this if needed
                //Creature * throwAsegfault = NULL;
                //throwAsegfault->getPosition();
                //exit(1);
                break;
        }
    }
}

void ODClient::sendToServer(ODPacket& packetToSend)
{
    send(packetToSend);
}

bool ODClient::connect(const std::string& host, const int port, const std::string& levelFilename)
{
    mLevelFilename = levelFilename;

    LogManager& logManager = LogManager::getSingleton();
    // Start the server socket listener as well as the server socket thread
    if (ODClient::getSingleton().isConnected())
    {
        logManager.logMessage("Couldn't try to connect: The client is already connected");
        return false;
    }

    // Start by loading map
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    if (gameMap == NULL)
        return false;

    // We set local player
    setPlayer(gameMap->getLocalPlayer());

    // Read in the map. The map loading should be happen here and not in the server thread to
    // make sure it is valid before launching the server.
    RenderManager::getSingletonPtr()->processRenderRequests();
    if (!gameMap->LoadLevel(levelFilename))
        return false;


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
        return false;

    if(!ODSocketClient::connect(host, port))
        return false;

    // Send a hello request to start the conversation with the server
    ODPacket packSend;
    packSend << ClientNotification::hello
        << std::string("OpenDungeons V ") + ODApplication::VERSION << mLevelFilename;
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
