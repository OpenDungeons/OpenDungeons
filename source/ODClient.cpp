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
    bool isNewTurn = false;
    // We loop until no more data is available
    while(isConnected() && processOneClientSocketMessage(isNewTurn));

   if(isNewTurn)
    {
        ODFrameListener::getSingleton().refreshChat();
    }
}

bool ODClient::processOneClientSocketMessage(bool& isNewTurn)
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();

    GameMap* gameMap = frameListener->getGameMap();
    if (!gameMap)
        return false;

    // Get a reference to the LogManager
    LogManager& logMgr = LogManager::getSingleton();

    ODPacket packetReceived;

    // Check if data available
    ODComStatus comStatus = recv(packetReceived);
    if(comStatus == ODComStatus::NotReady)
    {
        return false;
    }
    else if(comStatus == ODComStatus::Error)
    {
        // Place a chat message in the queue to inform
        // the user about the disconnect
        frameListener->addChatMessage(new ChatMessage("SERVER_INFORMATION: ",
            "Disconnected from server.", time(NULL)));
        // TODO : try to reconnect to the server
        return false;
    }

    std::string serverCommand;
    OD_ASSERT_TRUE(packetReceived >> serverCommand);

    // This if-else chain functions like a switch statement
    // on the command recieved from the server.

    if (serverCommand.compare(ServerNotification::typeString(ServerNotification::pickNick)) == 0)
    {
        ODPacket packSend;
        packSend << ClientNotification::typeString(ClientNotification::setNick) << gameMap->getLocalPlayer()->getNick();
        send(packSend);
    }
    else if (serverCommand.compare(ServerNotification::typeString(ServerNotification::yourSeat)) == 0)
    {
        int color;
        OD_ASSERT_TRUE(packetReceived >> color);
        Seat *tempSeat = gameMap->popEmptySeat(color);
        OD_ASSERT_TRUE(tempSeat != NULL);
        gameMap->getLocalPlayer()->setSeat(tempSeat);
    }

    else if (serverCommand.compare(ServerNotification::typeString(ServerNotification::addPlayer)) == 0)
    {
        Player *tempPlayer = new Player();
        std::string nick;
        int seatColor;
        OD_ASSERT_TRUE(packetReceived >> nick >> seatColor);
        tempPlayer->setNick(nick);
        Seat* seat = gameMap->popEmptySeat(seatColor);
        OD_ASSERT_TRUE(seat != NULL);
        gameMap->addPlayer(tempPlayer, seat);
    }

    else if (serverCommand.compare(ServerNotification::typeString(ServerNotification::chat)) == 0)
    {
        std::string chatNick;
        std::string chatMsg;
        OD_ASSERT_TRUE(packetReceived >> chatNick >> chatMsg);
        ChatMessage *newMessage = new ChatMessage(chatNick, chatMsg, time(NULL));
        frameListener->addChatMessage(newMessage);
    }

    else if (serverCommand.compare(ServerNotification::typeString(ServerNotification::newMap)) == 0)
    {
        gameMap->clearAll();
    }

    else if (serverCommand.compare(ServerNotification::typeString(ServerNotification::turnsPerSecond)) == 0)
    {
        OD_ASSERT_TRUE(packetReceived >> ODApplication::turnsPerSecond);
    }

    else if (serverCommand.compare(ServerNotification::typeString(ServerNotification::addTile)) == 0)
    {
        Tile newTile;
        OD_ASSERT_TRUE(packetReceived >> &newTile);
        newTile.setGameMap(gameMap);
        gameMap->addTile(newTile);

        // Loop over the tile's neighbors to force them to recheck
        // their mesh to see if they can use an optimized one
        std::vector<Tile*> neighbors = newTile.getAllNeighbors();
        for (unsigned int i = 0; i < neighbors.size(); ++i)
        {
            neighbors[i]->setFullness(neighbors[i]->getFullness());
        }
    }

    else if (serverCommand.compare(ServerNotification::typeString(ServerNotification::addMapLight)) == 0)
    {
        MapLight *newMapLight = new MapLight;
        OD_ASSERT_TRUE(packetReceived >> newMapLight);
        gameMap->addMapLight(newMapLight);
        newMapLight->createOgreEntity();
    }

    else if (serverCommand.compare(ServerNotification::typeString(ServerNotification::removeMapLight)) == 0)
    {
        std::string nameMapLight;
        OD_ASSERT_TRUE(packetReceived >> nameMapLight);
        MapLight *tempMapLight = gameMap->getMapLight(nameMapLight);
        tempMapLight->destroyOgreEntity();
        gameMap->removeMapLight(tempMapLight);
    }

    else if (serverCommand.compare(ServerNotification::typeString(ServerNotification::addClass)) == 0)
    {
        CreatureDefinition *tempClass = new CreatureDefinition;
        OD_ASSERT_TRUE(packetReceived >> tempClass);
        gameMap->addClassDescription(tempClass);
    }

    else if (serverCommand.compare(ServerNotification::typeString(ServerNotification::addCreature)) == 0)
    {
        //NOTE: This code is duplicated in readGameMapFromFile defined in src/Functions.cpp
        // Changes to this code should be reflected in that code as well
        Creature *newCreature = new Creature(gameMap);
        OD_ASSERT_TRUE(packetReceived >> newCreature);
        gameMap->addCreature(newCreature);
        newCreature->createMesh();
        newCreature->getWeaponL()->createMesh();
        newCreature->getWeaponR()->createMesh();
    }

    else if (serverCommand.compare(ServerNotification::typeString(ServerNotification::turnStarted)) == 0)
    {
        int64_t turnNum;
        OD_ASSERT_TRUE(packetReceived >> turnNum);
        logMgr.logMessage("Client received turnStarted="
            + Ogre::StringConverter::toString((int32_t)turnNum));
        gameMap->setTurnNumber(turnNum);

        isNewTurn = true;
        ODPacket packSend;
        packSend << ClientNotification::typeString(ClientNotification::ackNewTurn) << turnNum;
        send(packSend);
    }

    else if (serverCommand.compare(ServerNotification::typeString(ServerNotification::animatedObjectAddDestination)) == 0)
    {
        std::string objName;
        Ogre::Vector3 vect;
        OD_ASSERT_TRUE(packetReceived >> objName >> vect);
        MovableGameEntity *tempAnimatedObject = gameMap->getAnimatedObject(objName);

        if (tempAnimatedObject != NULL)
            tempAnimatedObject->addDestination(vect.x, vect.y);
    }

    else if (serverCommand.compare(ServerNotification::typeString(ServerNotification::animatedObjectClearDestinations)) == 0)
    {
        std::string objName;
        OD_ASSERT_TRUE(packetReceived >> objName);
        MovableGameEntity *tempAnimatedObject = gameMap->getAnimatedObject(objName);

        if (tempAnimatedObject != NULL)
            tempAnimatedObject->clearDestinations();
    }

    else if (serverCommand.compare(ServerNotification::typeString(ServerNotification::creaturePickedUp)) == 0)
    {
        int playerColor;
        std::string creatureName;
        OD_ASSERT_TRUE(packetReceived >> playerColor >> creatureName);
        Player *tempPlayer = gameMap->getPlayerByColor(playerColor);
        Creature *tempCreature = gameMap->getCreature(creatureName);

        if (tempPlayer != NULL && tempCreature != NULL)
        {
            tempPlayer->pickUpCreature(tempCreature);
        }
        else
        {
            logMgr.logMessage("Error while picking creature " + creatureName
                + " for player color=" + Ogre::StringConverter::toString(playerColor));
        }
    }

    else if (serverCommand.compare(ServerNotification::typeString(ServerNotification::creatureDropped)) == 0)
    {
        int playerColor;
        Tile tmpTile;
        OD_ASSERT_TRUE(packetReceived >> playerColor >> &tmpTile);
        Player *tempPlayer = gameMap->getPlayerByColor(playerColor);
        Tile* tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
        if (tempPlayer != NULL && tile != NULL)
        {
            tempPlayer->dropCreature(tile);
        }
        else
        {
            logMgr.logMessage("Error while dropping creature for player color="
                + Ogre::StringConverter::toString(playerColor)
                + " on tile " + Ogre::StringConverter::toString(tmpTile.getX())
                + "," + Ogre::StringConverter::toString(tmpTile.getY()));
        }
    }

    else if (serverCommand.compare(ServerNotification::typeString(ServerNotification::setObjectAnimationState)) == 0)
    {
        std::string objName;
        std::string animState;
        bool tempBool;
        OD_ASSERT_TRUE(packetReceived >> objName >> animState >> tempBool);
        MovableGameEntity *obj = gameMap->getAnimatedObject(objName);
        if (obj != NULL)
        {
            obj->setAnimationState(animState, tempBool);
        }
        else
        {
            logMgr.logMessage("ERROR: Server told us to change animations for a nonexistent object="
                + objName);
        }
    }

    else if (serverCommand.compare(ServerNotification::typeString(ServerNotification::tileFullnessChange)) == 0)
    {
        Tile tmpTile;

        OD_ASSERT_TRUE(packetReceived >> &tmpTile);

        Tile *tempTile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
        if (tempTile != NULL)
        {
            tempTile->setFullness(tmpTile.getFullness());
        }
        else
        {
            logMgr.logMessage("ERROR: Server told us to set the fullness for a nonexistent tile"
                + Ogre::StringConverter::toString(tmpTile.getX())
                + "," + Ogre::StringConverter::toString(tmpTile.getY()));
        }
    }

    else if (serverCommand.compare(ServerNotification::typeString(ServerNotification::tileClaimed)) == 0)
    {
        Tile tmpTile;
        OD_ASSERT_TRUE(packetReceived >> &tmpTile);

        Tile *tempTile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
        if (tempTile != NULL)
        {
            tempTile->claimTile(tmpTile.getColor());
        }
        else
        {
            logMgr.logMessage("ERROR: Server told us to set the color for a nonexistent tile="
                + Ogre::StringConverter::toString(tmpTile.getX()) + ","
                + Ogre::StringConverter::toString(tmpTile.getY()));
        }
    }

    else if(serverCommand.compare(ServerNotification::typeString(ServerNotification::refreshPlayerSeat)) == 0)
    {
        Seat tmpSeat;
        std::string goalsString;
        OD_ASSERT_TRUE(packetReceived >> &tmpSeat >> goalsString);
        getPlayer()->getSeat()->refreshFromSeat(&tmpSeat);
        ODFrameListener::getSingleton().refreshPlayerDisplay(goalsString);
    }

    else if(serverCommand.compare(ServerNotification::typeString(ServerNotification::markTiles)) == 0)
    {
        bool isDigSet;
        int nbTiles;
        OD_ASSERT_TRUE(packetReceived >> isDigSet >> nbTiles);
        std::vector<Tile*> tiles;
        for(int i = 0; i < nbTiles; i++)
        {
            Tile tmpTile;
            OD_ASSERT_TRUE(packetReceived >> &tmpTile);
            Tile* gameTile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            tiles.push_back(gameTile);
        }
        gameMap->markTilesForPlayer(tiles, isDigSet, gameMap->getLocalPlayer());
        SoundEffectsHelper::getSingleton().playInterfaceSound(SoundEffectsHelper::DIGSELECT, false);
    }

    else if(serverCommand.compare(ServerNotification::typeString(ServerNotification::buildRoom)) == 0)
    {
        int intType, color;
        Room::RoomType type;
        int nbTiles;
        OD_ASSERT_TRUE(packetReceived >> intType>> color >> nbTiles);
        type = static_cast<Room::RoomType>(intType);
        std::vector<Tile*> tiles;
        for(int i = 0; i < nbTiles; i++)
        {
            Tile tmpTile;
            OD_ASSERT_TRUE(packetReceived >> &tmpTile);
            Tile* gameTile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            tiles.push_back(gameTile);
        }
        Player* player = gameMap->getPlayerByColor(color);
        OD_ASSERT_TRUE(player != NULL);
        gameMap->buildRoomForPlayer(tiles, type, player);
    }

    else if(serverCommand.compare(ServerNotification::typeString(ServerNotification::buildTrap)) == 0)
    {
        int intType, color;
        Trap::TrapType type;
        int nbTiles;
        OD_ASSERT_TRUE(packetReceived >> intType >> color >> nbTiles);
        type = static_cast<Trap::TrapType>(intType);
        std::vector<Tile*> tiles;
        for(int i = 0; i < nbTiles; i++)
        {
            Tile tmpTile;
            OD_ASSERT_TRUE(packetReceived >> &tmpTile);
            Tile* gameTile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            tiles.push_back(gameTile);
        }
        Player* player = gameMap->getPlayerByColor(color);
        OD_ASSERT_TRUE(player != NULL);
        gameMap->buildTrapForPlayer(tiles, type, player);
    }

    else if(serverCommand.compare(ServerNotification::typeString(ServerNotification::addCreatureBed)) == 0)
    {
        Tile tmpTile;
        double xDim, yDim, rotationAngle;
        std::string roomName;
        std::string creatureName;
        OD_ASSERT_TRUE(packetReceived >> roomName >> creatureName >> &tmpTile
            >> xDim >> yDim >> rotationAngle);
        Room* room = gameMap->getRoomByName(roomName);
        OD_ASSERT_TRUE(room != NULL);
        Tile* tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
        OD_ASSERT_TRUE(tile != NULL);
        Creature* creature = gameMap->getCreature(creatureName);
        OD_ASSERT_TRUE(creature != NULL);
        RoomQuarters* rq = static_cast<RoomQuarters*>(room);
        OD_ASSERT_TRUE(rq->installBed(tile, creature, xDim, yDim, rotationAngle));
    }

    else if(serverCommand.compare(ServerNotification::typeString(ServerNotification::removeCreatureBed)) == 0)
    {
        Tile tmpTile;
        std::string roomName;
        std::string creatureName;
        OD_ASSERT_TRUE(packetReceived >> roomName >> creatureName >> &tmpTile);
        Room* room = gameMap->getRoomByName(roomName);
        OD_ASSERT_TRUE(room != NULL);
        Tile* tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
        OD_ASSERT_TRUE(tile != NULL);
        Creature* creature = gameMap->getCreature(creatureName);
        OD_ASSERT_TRUE(creature != NULL);
        RoomQuarters* rq = static_cast<RoomQuarters*>(room);
        OD_ASSERT_TRUE(rq->removeBed(tile, creature));
    }

    else if(serverCommand.compare(ServerNotification::typeString(ServerNotification::createTreasuryIndicator)) == 0)
    {
        int color;
        std::string roomName;
        Tile tmpTile;
        std::string indicatorMeshName;
        OD_ASSERT_TRUE(packetReceived >> color >> roomName >> &tmpTile >> indicatorMeshName);
        Room* room = gameMap->getRoomByName(roomName);
        OD_ASSERT_TRUE(room != NULL);
        Tile* tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
        OD_ASSERT_TRUE(tile != NULL);
        RoomTreasury* rt = static_cast<RoomTreasury*>(room);
        OD_ASSERT_TRUE(rt->getColor() == color);
        rt->createMeshesForTile(tile, indicatorMeshName);
    }

    else if(serverCommand.compare(ServerNotification::typeString(ServerNotification::destroyTreasuryIndicator)) == 0)
    {
        int color;
        std::string roomName;
        Tile tmpTile;
        std::string indicatorMeshName;
        OD_ASSERT_TRUE(packetReceived >> color >> roomName >> &tmpTile >> indicatorMeshName);
        Room* room = gameMap->getRoomByName(roomName);
        OD_ASSERT_TRUE(room != NULL);
        Tile* tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
        OD_ASSERT_TRUE(tile != NULL);
        RoomTreasury* rt = static_cast<RoomTreasury*>(room);
        OD_ASSERT_TRUE(rt->getColor() == color);
        rt->destroyMeshesForTile(tile, indicatorMeshName);
    }

    else
    {
        logMgr.logMessage("ERROR:  Unknown server command:" + serverCommand);
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
    bool running = true;

    while (running)
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

            case ClientNotification::exit:
                running = false;
                disconnect();
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
    if (ODServer::getSingleton().isConnected())
    {
        logManager.logMessage("Couldn't try to connect: The server is already connected");
        return false;
    }
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
    packSend << ClientNotification::typeString(ClientNotification::hello)
        << std::string("OpenDungeons V ") + ODApplication::VERSION << mLevelFilename;
    sendToServer(packSend);

    // Setup is finished. We can set the client to not blocking to allow to check
    // if the server sent a message
    setBlocking(false);
    return true;
}

void ODClient::queueClientNotification(ClientNotification* n)
{
    mClientNotificationQueue.push_back(n);
}
void ODClient::notifyExit()
{
    while(!mClientNotificationQueue.empty())
    {
        delete mClientNotificationQueue.front();
        mClientNotificationQueue.pop_front();
    }
    ClientNotification* exitClientNotification = new ClientNotification(
        ClientNotification::exit);
    queueClientNotification(exitClientNotification);
}
