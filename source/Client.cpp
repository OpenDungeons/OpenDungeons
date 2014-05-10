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

#include "Socket.h"
#include "ODFrameListener.h"
#include "Network.h"
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

void processClientSocketMessages()
{
    Socket* sock = Socket::clientSocket;

    if (!sock)
        return;

    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();

    GameMap* gameMap = frameListener->getGameMap();
    if (!gameMap)
        return;

    // Get a reference to the LogManager
    LogManager& logMgr = LogManager::getSingleton();

    if (!sock->is_valid())
        return;

    std::string commandFromServer;
    bool packetComplete = false;
    std::string tempString;

    // Loop until we get to a place that ends in a '>' symbol
    // indicating that we have 1 or more FULL messages so we
    // don't break in the middle of a message.
    while (!packetComplete)
    {
        int charsRead = sock->recv(tempString);
        // If the server closed the connection
        if (charsRead <= 0)
        {
            // Place a chat message in the queue to inform
            // the user about the disconnect
            frameListener->addChatMessage(new ChatMessage("SERVER_INFORMATION: ", "Server disconnect.", time(NULL)));

            return;
        }

        // Check to see if one or more complete packets in the buffer
        //FIXME:  This needs to be updated to include escaped closing brackets
        commandFromServer += tempString;
        if (commandFromServer[commandFromServer.length() - 1] == '>')
        {
            packetComplete = true;
        }
    }

    bool tempBool = false;
    std::string serverCommand;
    std::string arguments;

    // Parse a command out of the bytestream from the server.
    //NOTE: This command is duplicated at the end of this do-while loop.
    bool parseReturnValue = parseCommand(commandFromServer, serverCommand, arguments);
    do
    {
        // This if-else chain functions like a switch statement
        // on the command recieved from the server.

        if (serverCommand.compare("picknick") == 0)
        {
            sock->send(formatCommand("setnick", gameMap->getLocalPlayer()->getNick()));
        }

        /*
            else if(serverCommand.compare("yourseat") == 0)
            {
            std::stringstream tempSS(arguments);
            Seat *tempSeat = new Seat;
            cout << "\nAbout to read in seat.\n";
            cout.flush();
            tempSS >> tempSeat;
            gameMap->getLocalPlayer()->seat = tempSeat;
            }
            */

        else if (serverCommand.compare("addseat") == 0)
        {
            std::stringstream tempSS(arguments);
            Seat *tempSeat = new Seat;
            tempSS >> tempSeat;
            gameMap->addEmptySeat(tempSeat);
            if (gameMap->getLocalPlayer()->getSeat() == NULL)
            {
                gameMap->getLocalPlayer()->setSeat(gameMap->popEmptySeat());
            }

            sock->send(formatCommand("ok", "addseat"));
        }

        else if (serverCommand.compare("addplayer") == 0)
        {
            Player *tempPlayer = new Player();
            tempPlayer->setNick(arguments);
            gameMap->addPlayer(tempPlayer);

            sock->send(formatCommand("ok", "addplayer"));
        }

        else if (serverCommand.compare("chat") == 0)
        {
            ChatMessage *newMessage = processChatMessage(arguments);
            frameListener->addChatMessage(newMessage);
        }

        else if (serverCommand.compare("newmap") == 0)
        {
            gameMap->clearAll();
        }

        else if (serverCommand.compare("turnsPerSecond") == 0)
        {
            ODApplication::turnsPerSecond = atof(arguments.c_str());
        }

        else if (serverCommand.compare("addtile") == 0)
        {
            std::stringstream tempSS(arguments);
            Tile newTile;
            tempSS >> &newTile;
            newTile.setGameMap(gameMap);
            gameMap->addTile(newTile);

            sock->send(formatCommand("ok", "addtile"));

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
            std::stringstream tempSS(arguments);
            MapLight *newMapLight = new MapLight;
            tempSS >> newMapLight;
            gameMap->addMapLight(newMapLight);
            newMapLight->createOgreEntity();
        }

        else if (serverCommand.compare("removeMapLight") == 0)
        {
            MapLight *tempMapLight = gameMap->getMapLight(arguments);
            tempMapLight->destroyOgreEntity();
            gameMap->removeMapLight(tempMapLight);
        }

        else if (serverCommand.compare("addroom") == 0)
        {
            std::stringstream tempSS(arguments);
            std::string roomName;
            tempSS >> roomName;
            Room *newRoom = Room::createRoomFromStream(roomName, tempSS, gameMap);
            gameMap->addRoom(newRoom);
            newRoom->createMesh();

            sock->send(formatCommand("ok", "addroom"));
        }

        else if (serverCommand.compare("addclass") == 0)
        {
            std::stringstream tempSS(arguments);
            CreatureDefinition *tempClass = new CreatureDefinition;

            tempSS >> tempClass;

            gameMap->addClassDescription(tempClass);
            sock->send(formatCommand("ok", "addclass"));
        }

        else if (serverCommand.compare("addcreature") == 0)
        {
            //NOTE: This code is duplicated in readGameMapFromFile defined in src/Functions.cpp
            // Changes to this code should be reflected in that code as well
            Creature *newCreature = new Creature(gameMap);

            std::stringstream tempSS;
            tempSS.str(arguments);
            tempSS >> newCreature;

            gameMap->addCreature(newCreature);
            newCreature->createMesh();
            newCreature->getWeaponL()->createMesh();
            newCreature->getWeaponR()->createMesh();

            sock->send(formatCommand("ok", "addcreature"));
        }

        else if (serverCommand.compare("newturn") == 0)
        {
            std::stringstream tempSS;
            tempSS.str(arguments);
            long int tempLongInt;
            tempSS >> tempLongInt;
            gameMap->setTurnNumber(tempLongInt);
        }

        else if (serverCommand.compare("animatedObjectAddDestination") == 0)
        {
            char array[255];

            std::stringstream tempSS;
            tempSS.str(arguments);

            tempSS.getline(array, sizeof(array), ':');
            //tempString = array;
            MovableGameEntity *tempAnimatedObject = gameMap->getAnimatedObject(array);

            double tempX, tempY, tempZ;
            tempSS.getline(array, sizeof(array), ':');
            tempX = atof(array);
            tempSS.getline(array, sizeof(array), ':');
            tempY = atof(array);
            tempSS.getline(array, sizeof(array));
            tempZ = atof(array);

            Ogre::Vector3 tempVector((Ogre::Real)tempX, (Ogre::Real)tempY, (Ogre::Real)tempZ);

            if (tempAnimatedObject != NULL)
                tempAnimatedObject->addDestination(tempVector.x,
                        tempVector.y);
        }

        else if (serverCommand.compare("animatedObjectClearDestinations") == 0)
        {
            MovableGameEntity *tempAnimatedObject = gameMap->getAnimatedObject(arguments);

            if (tempAnimatedObject != NULL)
                tempAnimatedObject->clearDestinations();
        }

        //NOTE:  This code is duplicated in serverSocketProcessor()
        else if (serverCommand.compare("creaturePickUp") == 0)
        {
            char array[255];

            std::stringstream tempSS;
            tempSS.str(arguments);

            tempSS.getline(array, sizeof(array), ':');
            std::string playerNick = array;
            tempSS.getline(array, sizeof(array));
            std::string creatureName = array;

            Player *tempPlayer = gameMap->getPlayer(playerNick);
            Creature *tempCreature = gameMap->getCreature(creatureName);

            if (tempPlayer != NULL && tempCreature != NULL)
            {
                tempPlayer->pickUpCreature(tempCreature);
            }
        }

        //NOTE:  This code is duplicated in serverSocketProcessor()
        else if (serverCommand.compare("creatureDrop") == 0)
        {
            char array[255];

            std::stringstream tempSS;
            tempSS.str(arguments);

            tempSS.getline(array, sizeof(array), ':');
            std::string playerNick = array;
            tempSS.getline(array, sizeof(array), ':');
            int tempX = atoi(array);
            tempSS.getline(array, sizeof(array));
            int tempY = atoi(array);

            Player *tempPlayer = gameMap->getPlayer(playerNick);
            Tile *tempTile = gameMap->getTile(tempX, tempY);

            if (tempPlayer != NULL && tempTile != NULL)
            {
                tempPlayer->dropCreature(tempTile);
            }
        }

        else if (serverCommand.compare("setObjectAnimationState") == 0)
        {

            char array[255];
            std::string tempState;
            std::stringstream tempSS;
            tempSS.str(arguments);

            // Parse the creature name and get a pointer to it
            tempSS.getline(array, sizeof(array), ':');
            Creature *tempCreature = gameMap->getCreature(array);

            // Parse the animation state
            tempSS.getline(array, sizeof(array), ':');
            tempState = array;

            tempSS.getline(array, sizeof(array));
            tempString = array;
            tempBool = (tempString.compare("true") == 0);

            if (tempCreature != NULL)
            {
                tempCreature->setAnimationState(tempState, tempBool);
            }

        }

        else if (serverCommand.compare("tileFullnessChange") == 0)
        {
            char array[255];
            double tempFullness, tempX, tempY;
            std::stringstream tempSS;

            tempSS.str(arguments);
            tempSS.getline(array, sizeof(array), ':');
            tempFullness = atof(array);
            tempSS.getline(array, sizeof(array), ':');
            tempX = atof(array);
            tempSS.getline(array, sizeof(array));
            tempY = atof(array);

            Tile *tempTile = gameMap->getTile((int)tempX, (int)tempY);
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
            std::stringstream tempSS;
            tempSS.str("");
            tempSS << "ERROR:  Unknown server command!\nCommand:"
                    << serverCommand << std::endl << "Arguments:" << arguments << std::endl;
            logMgr.logMessage(tempSS.str());
        }

        //NOTE: This command is duplicated at the beginning of this do-while loop.
        parseReturnValue = parseCommand(commandFromServer, serverCommand, arguments);
    } while (parseReturnValue);
}

void processClientNotifications()
{
    std::stringstream tempSS;
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

                tempSS.str("");
                tempSS << tempPlayer->getNick() << ":" << tempCreature->getName();

                Socket::clientSocket->send(formatCommand("creaturePickUp", tempSS.str()));
                break;

            case ClientNotification::creatureDrop:
                tempPlayer = static_cast<Player*>(event->mP);
                tempTile = static_cast<Tile*>(event->mP2);

                tempSS.str("");
                tempSS << tempPlayer->getNick() << ":" << tempTile->x << ":"
                        << tempTile->y;

                Socket::clientSocket->send(formatCommand("creatureDrop", tempSS.str()));
                break;

            case ClientNotification::markTile:
                tempTile = static_cast<Tile*>(event->mP);
                flag = event->mFlag;
                tempSS.str("");
                tempSS << tempTile->x << ":" << tempTile->y << ":"
                        << (flag ? "true" : "false");

                Socket::clientSocket->send(formatCommand("markTile", tempSS.str()));
                break;

            case ClientNotification::exit:
                running = false;
                break;

            case ClientNotification::invalidType:
            default:
                LogManager& logMgr = LogManager::getSingleton();
                ODApplication::displayErrorMessage("Unhandled ClientNotification type encoutered!");
                logMgr.logMessage("Unhandled ClientNotification type encountered:");
                tempSS.str("");
                tempSS << (int) event->mType << std::endl;
                logMgr.logMessage(tempSS.str());

                // This is forcing a core dump so I can debug why this happened
                // Enable this if needed
                //Creature * throwAsegfault = NULL;
                //throwAsegfault->getPosition();
                //exit(1);
                break;
        }
    }
}
