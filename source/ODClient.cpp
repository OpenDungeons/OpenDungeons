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
#include "ODFrameListener.h"
#include "ChatMessage.h"
#include "GameMap.h"
#include "Seat.h"
#include "Player.h"
#include "MapLight.h"
#include "Creature.h"
#include "ClientNotification.h"
#include "Weapon.h"
#include "ODApplication.h"
#include "LogManager.h"

#include <string>

template<> ODClient* Ogre::Singleton<ODClient>::msSingleton = 0;

ODClient::ODClient() :
    ODSocketClient(false)
{
}

ODClient::~ODClient()
{
}


void ODClient::processClientSocketMessages()
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();

    GameMap* gameMap = frameListener->getGameMap();
    if (!gameMap)
        return;

    // Get a reference to the LogManager
    LogManager& logMgr = LogManager::getSingleton();

    ODPacket packetReceived;

    // Check if data available
    ODComStatus comStatus = recv(packetReceived);
    if(comStatus == ODComStatus::NotReady)
    {
        return;
    }
    else if(comStatus == ODComStatus::Error)
    {
        // Place a chat message in the queue to inform
        // the user about the disconnect
        frameListener->addChatMessage(new ChatMessage("SERVER_INFORMATION: ",
            "Disconnected from server.", time(NULL)));
        // TODO : try to reconnect to the server
        return;
    }

    std::string serverCommand;
    OD_ASSERT_TRUE(packetReceived >> serverCommand);

    // This if-else chain functions like a switch statement
    // on the command recieved from the server.

    if (serverCommand.compare("picknick") == 0)
    {
        ODPacket packSend;
        packSend << "setnick" << gameMap->getLocalPlayer()->getNick();
        send(packSend);
    }
    else if (serverCommand.compare("addseat") == 0)
    {
        Seat *tempSeat = new Seat;
        OD_ASSERT_TRUE(packetReceived >> tempSeat);
        gameMap->addEmptySeat(tempSeat);
        if (gameMap->getLocalPlayer()->getSeat() == NULL)
        {
            gameMap->getLocalPlayer()->setSeat(gameMap->popEmptySeat(tempSeat->getColor()));
        }

        ODPacket packSend;
        packSend << "ok" << "addseat";
        send(packSend);
    }

    else if (serverCommand.compare("addplayer") == 0)
    {
        Player *tempPlayer = new Player();
        std::string nick;
        int seatColor;
        OD_ASSERT_TRUE(packetReceived >> nick >> seatColor);
        tempPlayer->setNick(nick);
        gameMap->addPlayer(tempPlayer, gameMap->popEmptySeat(seatColor));

        ODPacket packSend;
        packSend << "ok" << "addplayer";
        send(packSend);
    }

    else if (serverCommand.compare("chat") == 0)
    {
        std::string chatNick;
        std::string chatMsg;
        OD_ASSERT_TRUE(packetReceived >> chatNick >> chatMsg);
        ChatMessage *newMessage = new ChatMessage(chatNick, chatMsg, time(NULL));
        frameListener->addChatMessage(newMessage);
    }

    else if (serverCommand.compare("newmap") == 0)
    {
        gameMap->clearAll();
    }

    else if (serverCommand.compare("turnsPerSecond") == 0)
    {
        OD_ASSERT_TRUE(packetReceived >> ODApplication::turnsPerSecond);
    }

    else if (serverCommand.compare("addtile") == 0)
    {
        Tile newTile;
        OD_ASSERT_TRUE(packetReceived >> &newTile);
        newTile.setGameMap(gameMap);
        gameMap->addTile(newTile);

        ODPacket packSend;
        packSend << "ok" << "addtile";
        send(packSend);

        // Loop over the tile's neighbors to force them to recheck
        // their mesh to see if they can use an optimized one
        std::vector<Tile*> neighbors = newTile.getAllNeighbors();
        for (unsigned int i = 0; i < neighbors.size(); ++i)
        {
            neighbors[i]->setFullness(neighbors[i]->getFullness());
        }
    }

    else if (serverCommand.compare("addmaplight") == 0)
    {
        MapLight *newMapLight = new MapLight;
        OD_ASSERT_TRUE(packetReceived >> newMapLight);
        gameMap->addMapLight(newMapLight);
        newMapLight->createOgreEntity();
    }

    else if (serverCommand.compare("removeMapLight") == 0)
    {
        std::string nameMapLight;
        OD_ASSERT_TRUE(packetReceived >> nameMapLight);
        MapLight *tempMapLight = gameMap->getMapLight(nameMapLight);
        tempMapLight->destroyOgreEntity();
        gameMap->removeMapLight(tempMapLight);
    }

    else if (serverCommand.compare("addroom") == 0)
    {
        std::string roomName;
        OD_ASSERT_TRUE(packetReceived >> roomName);
        Room *newRoom = Room::createRoomFromPacket(roomName, packetReceived, gameMap);
        gameMap->addRoom(newRoom);
        newRoom->createMesh();

        ODPacket packSend;
        packSend << "ok" << "addroom";
        send(packSend);
    }

    else if (serverCommand.compare("addclass") == 0)
    {
        CreatureDefinition *tempClass = new CreatureDefinition;
        OD_ASSERT_TRUE(packetReceived >> tempClass);
        gameMap->addClassDescription(tempClass);

        ODPacket packSend;
        packSend << "ok" << "addclass";
        send(packSend);
    }

    else if (serverCommand.compare("addcreature") == 0)
    {
        //NOTE: This code is duplicated in readGameMapFromFile defined in src/Functions.cpp
        // Changes to this code should be reflected in that code as well
        Creature *newCreature = new Creature(gameMap);
        OD_ASSERT_TRUE(packetReceived >> newCreature);
        gameMap->addCreature(newCreature);
        newCreature->createMesh();
        newCreature->getWeaponL()->createMesh();
        newCreature->getWeaponR()->createMesh();

        ODPacket packSend;
        packSend << "ok" << "addcreature";
        send(packSend);
    }

    else if (serverCommand.compare("newturn") == 0)
    {
        int64_t turnNum;
        OD_ASSERT_TRUE(packetReceived >> turnNum);
        gameMap->setTurnNumber(turnNum);
    }

    else if (serverCommand.compare("animatedObjectAddDestination") == 0)
    {
        std::string objName;
        OD_ASSERT_TRUE(packetReceived >> objName);
        MovableGameEntity *tempAnimatedObject = gameMap->getAnimatedObject(objName);

        double tempX, tempY, tempZ;
        OD_ASSERT_TRUE(packetReceived >> tempX >> tempY >> tempZ);
        Ogre::Vector3 tempVector((Ogre::Real)tempX, (Ogre::Real)tempY, (Ogre::Real)tempZ);

        if (tempAnimatedObject != NULL)
            tempAnimatedObject->addDestination(tempVector.x,
                    tempVector.y);
    }

    else if (serverCommand.compare("animatedObjectClearDestinations") == 0)
    {
        std::string objName;
        OD_ASSERT_TRUE(packetReceived >> objName);
        MovableGameEntity *tempAnimatedObject = gameMap->getAnimatedObject(objName);

        if (tempAnimatedObject != NULL)
            tempAnimatedObject->clearDestinations();
    }

    //NOTE:  This code is duplicated in serverSocketProcessor()
    else if (serverCommand.compare("creaturePickUp") == 0)
    {
        std::string playerNick;
        std::string creatureName;
        OD_ASSERT_TRUE(packetReceived >> playerNick >> creatureName);
        Player *tempPlayer = gameMap->getPlayer(playerNick);
        Creature *tempCreature = gameMap->getCreature(creatureName);

        if (tempPlayer != NULL && tempCreature != NULL)
        {
            tempPlayer->pickUpCreature(tempCreature);
        }
        else
        {
            logMgr.logMessage("Error while picking creature " + creatureName
                + " for player " + playerNick);
        }
    }

    //NOTE:  This code is duplicated in serverSocketProcessor()
    else if (serverCommand.compare("creatureDrop") == 0)
    {
        std::string playerNick;
        int tempX;
        int tempY;
        OD_ASSERT_TRUE(packetReceived >> playerNick >> tempX >> tempY);

        Player *tempPlayer = gameMap->getPlayer(playerNick);
        Tile *tempTile = gameMap->getTile(tempX, tempY);

        if (tempPlayer != NULL && tempTile != NULL)
        {
            tempPlayer->dropCreature(tempTile);
        }
        else
        {
            logMgr.logMessage("Error while dropping creature for player " + playerNick
                + " on tile " + Ogre::StringConverter::toString(tempX)
                + "," + Ogre::StringConverter::toString(tempY));
        }
    }

    else if (serverCommand.compare("setObjectAnimationState") == 0)
    {
        // Parse the creature name and get a pointer to it
        std::string creatureName;
        std::string animState;
        bool tempBool;
        OD_ASSERT_TRUE(packetReceived >> creatureName >> animState >> tempBool);

        Creature *tempCreature = gameMap->getCreature(creatureName);
        if (tempCreature != NULL)
        {
            tempCreature->setAnimationState(animState, tempBool);
        }

    }

    else if (serverCommand.compare("tileFullnessChange") == 0)
    {
        double tempFullness;
        int tempX, tempY;

        OD_ASSERT_TRUE(packetReceived >> tempFullness >> tempX >> tempY);

        Tile *tempTile = gameMap->getTile(tempX, tempY);
        if (tempTile != NULL)
        {
            tempTile->setFullness(tempFullness);
        }
        else
        {
            logMgr.logMessage("ERROR:  Server told us to set the fullness for a nonexistent tile.");
        }
    }

    else
    {
        logMgr.logMessage("ERROR:  Unknown server command!\nCommand:"
            + serverCommand);
    }
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
        if (ClientNotification::mClientNotificationQueue.empty())
            break;

        ClientNotification* event = ClientNotification::mClientNotificationQueue.front();
        ClientNotification::mClientNotificationQueue.pop_front();

        if (!event)
            continue;

        switch (event->mType)
        {
            case ClientNotification::creaturePickUp:
                tempCreature = static_cast<Creature*>(event->mP);
                tempPlayer = static_cast<Player*>(event->mP2);

                packSend.clear();
                packSend << "creaturePickUp" << tempPlayer->getNick() << tempCreature->getName();
                sendToServer(packSend);
                break;

            case ClientNotification::creatureDrop:
                tempPlayer = static_cast<Player*>(event->mP);
                tempTile = static_cast<Tile*>(event->mP2);

                packSend.clear();
                packSend << "creatureDrop" << tempPlayer->getNick() << tempTile->x << tempTile->y;
                sendToServer(packSend);
                break;

            case ClientNotification::markTile:
                tempTile = static_cast<Tile*>(event->mP);
                flag = event->mFlag;

                packSend.clear();
                packSend << "markTile" << tempTile->x << tempTile->y << flag;
                sendToServer(packSend);
                break;

            case ClientNotification::exit:
                running = false;
                break;

            case ClientNotification::invalidType:
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

