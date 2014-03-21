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

//TODO: get rid of this whole file.
//      - The server stuff should go into some of the network classes.

#include "Functions.h"

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

    sem_wait(&ServerNotification::serverNotificationQueueLockSemaphore);
    ServerNotification::serverNotificationQueue.push_back(n);
    sem_post(&ServerNotification::serverNotificationQueueLockSemaphore);

    sem_post(&ServerNotification::serverNotificationQueueSemaphore);
}

bool startServer()
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
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
    if (gameMap->numEmptySeats() == 0)
    {
        logManager.logMessage("Couldn't start server: The number of empty seats was 0.");
        return false;
    }

    //NOTE: Code added to this routine may also need to be added to GameMap::doTurn() in the "loadNextLevel" stuff.
    // Sit down at the first available seat.
    gameMap->getLocalPlayer()->setSeat(gameMap->popEmptySeat());

    //NOTE - temporary code to test ai
    Player* aiPlayer = new Player();
    aiPlayer->setNick("test ai player");
    bool success = gameMap->addPlayer(aiPlayer);
    if(!success)
    {
        logManager.logMessage("Failed to add player");
    }
    else
    {
        success = gameMap->assignAI(*aiPlayer, "testai");
        if(!success)
        {
            logManager.logMessage("Failed to assign ai to player");
        }
    }

    logManager.logMessage("Player has colour:" +
        Ogre::StringConverter::toString(gameMap->getLocalPlayer()->getSeat()->getColor()));
    logManager.logMessage("ai has colour:" +
        Ogre::StringConverter::toString(aiPlayer->getSeat()->getColor()));

    Socket::serverSocket = new Socket;

    // Start the server thread which will listen for, and accept, connections
    SSPStruct* ssps = new SSPStruct;
    ssps->nSocket = Socket::serverSocket;
    ssps->nFrameListener = ODFrameListener::getSingletonPtr();
    pthread_create(&ODFrameListener::getSingletonPtr()->serverThread,
                   NULL, serverSocketProcessor, (void*) ssps);
        
    //Move camera to dungeon temple (FIXME: This should probably not be done here.)
    // Seat* localPlayerSeat = gameMap.getLocalPlayer()->getSeat();
    // CameraManager::getSingleton().flyTo(Ogre::Vector3(localPlayerSeat->startingX, localPlayerSeat->startingY, 0.0)
// 				    );

    // Start the thread which will watch for local events to send to the clients
    SNPStruct* snps = new SNPStruct;
    snps->nFrameListener = ODFrameListener::getSingletonPtr();
    pthread_create(&ODFrameListener::getSingletonPtr()->serverNotificationThread,
                   NULL, serverNotificationProcessor, snps);

    // Start the creature AI thread
    pthread_create(&ODFrameListener::getSingletonPtr()->creatureThread,
                   NULL, creatureAIThread, static_cast<void*>(gameMap));

    // Destroy the meshes associated with the map lights that allow you to see/drag them in the map editor.
    gameMap->clearMapLightIndicators();

    return true;
}

} // namespace ODServer
