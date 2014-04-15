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
#include "MapLight.h"
#include "Seat.h"
#include "Player.h"
#include "ODFrameListener.h"
#include "LogManager.h"
#include "CameraManager.h"

#include <CEGUI/CEGUI.h>
#include <CEGUI/WindowManager.h>
#include <CEGUI/widgets/TabControl.h>

//TODO: Make a server class.
namespace ODServer {

void queueServerNotification(ServerNotification* n)
{
    if (n == NULL)
        return;

    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    if (gameMap == NULL)
        return;

    n->turnNumber = GameMap::turnNumber.get();
    gameMap->threadLockForTurn(n->turnNumber);

    sem_wait(&ServerNotification::mServerNotificationQueueLockSemaphore);
    ServerNotification::serverNotificationQueue.push_back(n);
    sem_post(&ServerNotification::mServerNotificationQueueLockSemaphore);

    sem_post(&ServerNotification::mServerNotificationQueueSemaphore);
}

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

    // Start the server thread which will listen for, and accept, connections
    SSPStruct* ssps = new SSPStruct;
    ssps->nSocket = Socket::serverSocket;
    ssps->nFrameListener = frameListener;
    if (frameListener->mServerThread == NULL)
    {
        frameListener->mServerThread = new pthread_t;
        pthread_create(frameListener->mServerThread,
                       NULL, serverSocketProcessor, (void*) ssps);
    }
    else
    {
        logManager.logMessage("Warning: Server thread already started when trying to create server.");
    }

    // Start the thread which will watch for local events to send to the clients
    SNPStruct* snps = new SNPStruct;
    snps->nFrameListener = frameListener;
    if (frameListener->mServerNotificationThread == NULL)
    {
        frameListener->mServerNotificationThread = new pthread_t;
        pthread_create(frameListener->mServerNotificationThread,
                       NULL, serverNotificationProcessor, snps);
    }
    else
    {
        logManager.logMessage("Warning: Server notification thread already started when trying to create server.");
    }

    // Start the creature AI thread
    if (frameListener->mCreatureThread == NULL)
    {
        frameListener->mCreatureThread = new pthread_t;
        pthread_create(frameListener->mCreatureThread,
                       NULL, creatureAIThread, static_cast<void*>(gameMap));
    }
    else
    {
        logManager.logMessage("Warning: Creature AI thread already started when trying to create server.");
    }

    return true;
}

} // namespace ODServer
