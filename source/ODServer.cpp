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

#include "Network.h"
#include "Socket.h"
#include "ServerNotification.h"
#include "GameMap.h"
#include "ODApplication.h"
#include "ODFrameListener.h"
#include "LogManager.h"

//TODO: Make a server class.
namespace ODServer {

bool startServer()
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    GameMap* gameMap = frameListener->getGameMap();
    if (gameMap == NULL)
        return false;

    LogManager& logManager = LogManager::getSingleton();

    // Start the server socket listener as well as the server socket thread
    if (Socket::serverSocket != NULL)
    {
        logManager.logMessage("Couldn't start server: The server socket was not NULL");
        return false;
    }
    if (Socket::clientSocket != NULL)
    {
        logManager.logMessage("Couldn't start server: The client socket was not NULL");
        return false;
    }

    Socket::serverSocket = new Socket;
    // Set up the socket to listen on the specified port
    if (!Socket::serverSocket->create())
    {
        delete Socket::serverSocket;
        Socket::serverSocket == NULL;
        logManager.logMessage("ERROR:  Server could not create server socket!");
        return false;
    }

    if (!Socket::serverSocket->bind(ODApplication::PORT_NUMBER))
    {
        delete Socket::serverSocket;
        Socket::serverSocket == NULL;
        logManager.logMessage("ERROR:  Server could not bind to port!");
        return false;
    }

    return true;
}

void queueServerNotification(ServerNotification* n)
{
    if (n == NULL)
        return;

    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    if (gameMap == NULL)
        return;

    n->turnNumber = GameMap::turnNumber.get();
    gameMap->threadLockForTurn(n->turnNumber);

    ServerNotification::serverNotificationQueue.push_back(n);
}

void processServerEvents()
{
    // Place a message in the queue to inform the clients that a new turn has started
    try
    {
        ServerNotification* serverNotification = new ServerNotification;
        serverNotification->type = ServerNotification::turnStarted;

        ODServer::queueServerNotification(serverNotification);
    }
    catch (bad_alloc&)
    {
        LogManager::getSingletonPtr()->logMessage("ERROR:  bad alloc in turnStarted", Ogre::LML_CRITICAL);
    }

    processServerSocketMessages();
    processServerNotifications();

    // Process each connected client notifications
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    std::vector<Socket*>::iterator it = frameListener->mClientSockets.begin();
    while (it != frameListener->mClientSockets.end())
    {
        Socket* clientSocket = *it;
        bool clientStillConnected = processClientNotifications(clientSocket);
        if (clientStillConnected)
        {
            ++it;
        }
        else // erase the disconnected client references
        {
            if (clientSocket)
                delete clientSocket;

            frameListener->mClientSockets.erase(it);
        }
    }
}

} // namespace ODServer
