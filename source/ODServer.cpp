/*
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

#include "ODServer.h"

#include "ODClient.h"
#include "ServerNotification.h"
#include "GameMap.h"
#include "ODApplication.h"
#include "ODFrameListener.h"
#include "MapLight.h"
#include "LogManager.h"

#include <SFML/Network.hpp>
#include <SFML/System.hpp>

template<> ODServer* Ogre::Singleton<ODServer>::msSingleton = 0;

ODServer::ODServer()
{

}

ODServer::~ODServer()
{

}

bool ODServer::startServer()
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();

    LogManager& logManager = LogManager::getSingleton();

    // Start the server socket listener as well as the server socket thread
    if (ODServer::getSingleton().isConnected())
    {
        logManager.logMessage("Couldn't start server: The server is already connected");
        return false;
    }
    if (ODClient::getSingleton().isConnected())
    {
        logManager.logMessage("Couldn't start server: The client is already connected");
        return false;
    }

    // Set up the socket to listen on the specified port
    if (!createServer(ODApplication::PORT_NUMBER))
    {
        logManager.logMessage("ERROR:  Server could not create server socket!");
        return false;
    }

    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    if (gameMap == NULL)
        return false;

    return true;
}

void ODServer::queueServerNotification(ServerNotification* n)
{
    if (n == NULL)
        return;

    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    if (gameMap == NULL)
        return;

    n->turnNumber = gameMap->getTurnNumber();

    ServerNotification::serverNotificationQueue.push_back(n);
}

void ODServer::processServerEvents()
{
    doTask(10);
    processServerNotifications();

}

void ODServer::processServerNotifications()
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    GameMap* gameMap = frameListener->getGameMap();

    if((gameMap->getTurnNumber() == 0) &&
       (gameMap->getEmptySeat("Player") == NULL))
    {
        // Every client is connected and ready, we can launch the game
        gameMap->setTurnNumber(1);
    }
    else if(gameMap->getTurnNumber() == -1)
    {
        // We are still waiting for players
        return;
    }


    // Place a message in the queue to inform the clients that a new turn has started
    try
    {
        ServerNotification* serverNotification = new ServerNotification;
        serverNotification->type = ServerNotification::turnStarted;

        queueServerNotification(serverNotification);
    }
    catch (std::bad_alloc&)
    {
        LogManager::getSingletonPtr()->logMessage("ERROR:  bad alloc in turnStarted", Ogre::LML_CRITICAL);
    }

    ODPacket packetSend;
    Tile *tempTile;
    Player *tempPlayer;
    MapLight *tempMapLight;
    MovableGameEntity *tempAnimatedObject;
    bool running = true;

    while (running)
    {
        // If the queue is empty, let's get out of the loop.
        if (ServerNotification::serverNotificationQueue.empty())
            break;

        // Take a message out of the front of the notification queue
        ServerNotification *event = ServerNotification::serverNotificationQueue.front();
        ServerNotification::serverNotificationQueue.pop_front();

        //FIXME:  This really should never happen but the queue does occasionally pop a NULL.
        //This is probably a bug somewhere else where a NULL is being place in the queue.
        if (event == NULL)
            continue;

        switch (event->type)
        {
            case ServerNotification::turnStarted:
                packetSend.clear();
                packetSend << "newturn" << gameMap->getTurnNumber();
                sendToAllClients(packetSend);
                break;

            case ServerNotification::animatedObjectAddDestination:
                packetSend.clear();
                packetSend << "animatedObjectAddDestination" << event->str << event->vec.x
                    << event->vec.y << event->vec.z;
                sendToAllClients(packetSend);
                break;

            case ServerNotification::animatedObjectClearDestinations:
                packetSend.clear();
                packetSend << "animatedObjectClearDestinations" << event->ani->getName();
                sendToAllClients(packetSend);
                break;

                //NOTE: this code is duplicated in clientNotificationProcessor
            case ServerNotification::creaturePickUp:
                packetSend.clear();
                packetSend << "creaturePickUp" << event->player->getNick()
                    << event->cre->getName();
                sendToAllClients(packetSend);
                break;

                //NOTE: this code is duplicated in clientNotificationProcessor
            case ServerNotification::creatureDrop:
                tempPlayer = event->player;
                tempTile = event->tile;

                packetSend.clear();
                packetSend << "creatureDrop" << tempPlayer->getNick() << tempTile->x
                    << tempTile->y;
                sendToAllClients(packetSend);
                break;

            case ServerNotification::setObjectAnimationState:
                tempAnimatedObject = static_cast<MovableGameEntity*>(event->p);
                packetSend.clear();
                packetSend << "setObjectAnimationState" << tempAnimatedObject->getName() << event->str
                    << ":" << event->b;
                sendToAllClients(packetSend);
                break;

            case ServerNotification::setTurnsPerSecond:
                packetSend.clear();
                packetSend << "turnsPerSecond" << ODApplication::turnsPerSecond;
                sendToAllClients(packetSend);
                break;

            case ServerNotification::tileFullnessChange:
                tempTile = event->tile;
                packetSend.clear();
                packetSend << "tileFullnessChange" << tempTile->getFullness() << tempTile->x
                    << tempTile->y;
                sendToAllClients(packetSend);
                break;

            case ServerNotification::addMapLight:
                tempMapLight = static_cast<MapLight*>(event->p);
                packetSend.clear();
                packetSend << "addmaplight" << tempMapLight;
                sendToAllClients(packetSend);
                break;

            case ServerNotification::removeMapLight:
                tempMapLight = static_cast<MapLight*>(event->p);
                packetSend.clear();
                packetSend << "removeMapLight" << tempMapLight->getName();
                sendToAllClients(packetSend);
                break;

            case ServerNotification::exit:
            {
                running = false;
                break;
            }

            default:
                std::cout << "\n\nERROR:  Unhandled ServerNotification type encoutered!\n\n";
                break;
        }

        delete event;
        event = NULL;
    }
}

bool ODServer::processClientNotifications(ODSocketClient* clientSocket)
{
    if (!clientSocket)
        return false;

    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    GameMap* gameMap = frameListener->getGameMap();
    if (!gameMap)
        return true;

    Player *curPlayer = NULL;

    std::string clientNick = "UNSET_CLIENT_NICKNAME";
    std::string clientCommand;
    ODPacket packetReceived;
    std::string tempString2;

    ODSocketClient::ODComStatus status = receiveMsgFromClient(clientSocket, packetReceived);

    // If the client closed the connection
    if (status != ODSocketClient::ODComStatus::OK)
    {
        return (status != ODSocketClient::ODComStatus::Error);
    }

    OD_ASSERT_TRUE(packetReceived >> clientCommand);

    if (clientCommand.compare("hello") == 0)
    {
        ODPacket packetSend;
        std::string version;
        OD_ASSERT_TRUE(packetReceived >> version);
        frameListener->addChatMessage(new ChatMessage(
                "SERVER_INFORMATION: ", "Client connect with version: "
                        + version, time(NULL)));

        // Tell the client to give us their nickname and to clear their map
        packetSend.clear();
        packetSend << "picknick";
        sendMsgToClient(clientSocket, packetSend);

        // Set the nickname that the client sends back, tempString2 is just used
        // to discard the command portion of the respone which should be "setnick"
        //TODO:  verify that this really is true
        packetReceived.clear();
        receiveMsgFromClient(clientSocket, packetReceived);
        // Throw the command
        OD_ASSERT_TRUE(packetReceived >> clientNick);
        if(clientNick != "setnick")
        {
            LogManager::getSingleton().logMessage("ERROR : Expected setnick from client but got "
                + clientNick);
        }
        // Pick nick
        OD_ASSERT_TRUE(packetReceived >> clientNick);

        frameListener->addChatMessage(new ChatMessage(
                "SERVER_INFORMATION: ", "Client nick is: " + clientNick,
                time(NULL)));

        // Create a player structure for the client
        //TODO:  negotiate and set a color
        curPlayer = new Player;
        curPlayer->setNick(clientNick);
        // The seat should be available since it has been checked before accepting the client connexion
        Seat* seat = gameMap->getEmptySeat("Player");
        if(seat != NULL)
        {
            gameMap->addPlayer(curPlayer, gameMap->popEmptySeat(seat->getColor()));
        }
        else
        {
            // No seat available. We disconnect the client
            return false;
        }

        packetSend.clear();
        packetSend << "newmap";
        sendMsgToClient(clientSocket, packetSend);

        // Tell the player which seat it has
        packetSend.clear();
        packetSend << "addseat" << curPlayer->getSeat();
        sendMsgToClient(clientSocket, packetSend);

        packetSend.clear();
        packetSend << "turnsPerSecond" << ODApplication::turnsPerSecond;
        sendMsgToClient(clientSocket, packetSend);

        // Send over the information about the players in the game
        packetSend.clear();
        packetSend << "addplayer" << gameMap->getLocalPlayer()->getNick()
            << gameMap->getLocalPlayer()->getSeat()->getColor();
        sendMsgToClient(clientSocket, packetSend);
        for (unsigned int i = 0; i < gameMap->numPlayers(); ++i)
        {
            // Don't tell the client about its own player structure
            Player* tempPlayer = gameMap->getPlayer(i);
            if (curPlayer != tempPlayer && tempPlayer != NULL)
            {
                packetSend.clear();
                packetSend << "addseat" << tempPlayer->getSeat();
                sendMsgToClient(clientSocket, packetSend);
                // Throw away the ok response
                packetReceived.clear();
                receiveMsgFromClient(clientSocket, packetReceived);

                packetSend.clear();
                packetSend << "addplayer" << tempPlayer->getNick()
                    << tempPlayer->getSeat()->getColor();
                sendMsgToClient(clientSocket, packetSend);
                // Throw away the ok response
                packetReceived.clear();
                receiveMsgFromClient(clientSocket, packetReceived);
            }
        }

        // Send over the map tiles from the current game map.
        //TODO: Only send the tiles which the client is supposed to see due to fog of war.
        for(int ii = 0; ii < gameMap->getMapSizeX(); ++ii)
        {
            for(int jj = 0; jj < gameMap->getMapSizeY(); ++jj)
            {
                packetSend.clear();
                packetSend << "addtile" << gameMap->getTile(ii,jj);
                sendMsgToClient(clientSocket, packetSend);
                // Throw away the ok response
                packetReceived.clear();
                receiveMsgFromClient(clientSocket, packetReceived);
            }
        }

        // Send over the map lights from the current game map.
        //TODO: Only send the maplights which the client is supposed to see due to the fog of war.
        for (unsigned int ii = 0; ii < gameMap->numMapLights(); ++ii)
        {
            packetSend.clear();
            packetSend << "addmaplight" << gameMap->getMapLight(ii);
            sendMsgToClient(clientSocket, packetSend);
        }

        // Send over the rooms in use on the current game map
        //TODO: Only send the classes which the client is supposed to see due to fog of war.
        for (unsigned int ii = 0; ii < gameMap->numRooms(); ++ii)
        {
            packetSend.clear();
            packetSend << "addroom" << gameMap->getRoom(ii);
            sendMsgToClient(clientSocket, packetSend);
            // Throw away the ok response
            packetReceived.clear();
            receiveMsgFromClient(clientSocket, packetReceived);
        }

        // Send over the class descriptions in use on the current game map
        // TODO: Only send the classes which the client is supposed to see due to fog of war.
        for (unsigned int i = 0; i < gameMap->numClassDescriptions(); ++i)
        {
            // NOTE: This code is duplicated in writeGameMapToFile defined in src/Functions.cpp
            // Changes to this code should be reflected in that code as well
            CreatureDefinition *tempClass = gameMap->getClassDescription(i);
            packetSend.clear();
            packetSend << "addclass" << tempClass;
            sendMsgToClient(clientSocket, packetSend);
            // Throw away the ok response
            //TODO:  Actually check this.
            packetReceived.clear();
            receiveMsgFromClient(clientSocket, packetReceived);
        }

        // Send over the actual creatures in use on the current game map
        //TODO: Only send the creatures which the client is supposed to see due to fog of war.
        for (unsigned int i = 0; i < gameMap->numCreatures(); ++i)
        {
            Creature *tempCreature = gameMap->getCreature(i);
            packetSend.clear();
            packetSend << "addcreature" << tempCreature;
            sendMsgToClient(clientSocket, packetSend);
            // Throw away the ok response
            packetReceived.clear();
            receiveMsgFromClient(clientSocket, packetReceived);
        }
    }

    else if (clientCommand.compare("chat") == 0)
    {
        std::string chatNick;
        std::string chatMsg;
        OD_ASSERT_TRUE(packetReceived >> chatNick >> chatMsg);
        ChatMessage *newMessage = new ChatMessage(chatNick, chatMsg, time(NULL));

        ODPacket packetSend;
        packetSend << "chat" << newMessage->getClientNick() << newMessage->getMessage();
        sendToAllClients(packetSend);

        // Put the message in our own queue
        frameListener->addChatMessage(newMessage);
    }

    //NOTE:  This code is duplicated in clientSocketProcessor()
    else if (clientCommand.compare("creaturePickUp") == 0)
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
            LogManager::getSingleton().logMessage("ERROR : player " + playerNick
                + " could not pick up creature " + creatureName);
        }
    }

    //NOTE:  This code is duplicated in clientSocketProcessor()
    else if (clientCommand.compare("creatureDrop") == 0)
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
            LogManager::getSingleton().logMessage("ERROR : player " + playerNick
                + " could not drop creature on tile "
                + Ogre::StringConverter::toString(tempX)
                + "," + Ogre::StringConverter::toString(tempY));
        }
    }

    else if (clientCommand.compare("markTile") == 0)
    {
        int tempX;
        int tempY;
        bool flag;

        OD_ASSERT_TRUE(packetReceived >> tempX >> tempY >> flag);

        Tile *tempTile = gameMap->getTile(tempX, tempY);
        if (tempTile != NULL)
        {
            Player *tempPlayer = gameMap->getPlayer(clientNick);
            if (tempPlayer != NULL)
            {
                tempTile->setMarkedForDigging(flag, tempPlayer);
            }
        }
    }

    else if (clientCommand.compare("ok") == 0)
    {
        std::string ack;
        OD_ASSERT_TRUE(packetReceived >> ack);
        std::cout << "\nIgnoring an ak message from a client: " << ack;
    }

    else
    {
        std::cerr << "\n\nERROR:  Unhandled command recieved from client:\nCommand:  ";
        std::cerr << clientCommand << "\n\n";
    }

    /*if(frameListener->getThreadStopRequested())
    {
        //TODO - log
        break;
    }*/
    return true;
}

void ODServer::sendToAllClients(ODPacket& packetSend)
{
    // TODO : except the console, nothing should decide to whom a cmd should be sent. Check if it is true
    sendMsgToAllClients(packetSend);
}

bool ODServer::notifyNewConnection(ODSocketClient *sock)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    if (gameMap == NULL)
        return false;

    // TODO : the seat should be pop and saved in the client so that we can keep track if several clients
    // connects at same time (otherwise, clients could be accepted even if no seat available)
    Seat* seat = gameMap->getEmptySeat("Player");
    return (seat != NULL);
}

void ODServer::notifyClientMessage(ODSocketClient *client)
{
    if(!processClientNotifications(client))
    {
        // TODO : send a message to every client
        ODFrameListener::getSingletonPtr()->addChatMessage(new ChatMessage("SERVER_INFORMATION: ",
            "Client disconnected", time(NULL)));
        clearClientSocket(client);
        // TODO : wait for a new connection if the client reconnects
    }
}
