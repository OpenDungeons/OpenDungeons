//TODO: get rid of this whole file.
//      - The server stuff should go into some of the network classes.

#include <CEGUI.h>

#include "Network.h"
#include "Socket.h"
#include "ServerNotification.h"
#include "GameMap.h"
#include "MapLight.h"
#include "Seat.h"
#include "Player.h"
#include "ODFrameListener.h"
#include "LogManager.h"

#include "Functions.h"
#include "CameraManager.h"

void queueServerNotification(ServerNotification *n)
{
    //TODO: Make a server class.
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    n->turnNumber = GameMap::turnNumber.get();
    gameMap->threadLockForTurn(n->turnNumber);

    sem_wait(&ServerNotification::serverNotificationQueueLockSemaphore);
    ServerNotification::serverNotificationQueue.push_back(n);
    sem_post(&ServerNotification::serverNotificationQueueLockSemaphore);

    sem_post(&ServerNotification::serverNotificationQueueSemaphore);
}

bool startServer(GameMap& gameMap)
{
    // Start the server socket listener as well as the server socket thread
    if (Socket::serverSocket == NULL && Socket::clientSocket == NULL && gameMap.numEmptySeats()
            > 0)
    {
        LogManager& logManager = LogManager::getSingleton();
        //NOTE: Code added to this routine may also need to be added to GameMap::doTurn() in the "loadNextLevel" stuff.
        // Sit down at the first available seat.
        gameMap.getLocalPlayer()->setSeat(gameMap.popEmptySeat());

        //NOTE - temporary code to test ai
        Player* aiPlayer = new Player();
        aiPlayer->setNick("test ai player");
        bool success = gameMap.addPlayer(aiPlayer);
        if(!success)
        {
            logManager.logMessage("Failed to add player");
        }
        else
        {
            success = gameMap.assignAI(*aiPlayer, "testai");
            if(!success)
            {
                logManager.logMessage("Failed to assign ai to player");
            }
        }

        logManager.logMessage("Player has colour:" +
            Ogre::StringConverter::toString(gameMap.getLocalPlayer()->getSeat()->getColor()));
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
                NULL, creatureAIThread, static_cast<void*>(&gameMap));

        // Destroy the meshes associated with the map lights that allow you to see/drag them in the map editor.
        gameMap.clearMapLightIndicators();

        // Set the active tabs on the tab selector across the bottom of the screen so
        // the user doesn't have to click into them first to see the contents.
        static_cast<CEGUI::TabControl*>(CEGUI::WindowManager::getSingletonPtr()->
                getWindow("Root/MainTabControl"))->setSelectedTab(0);
    }

    return true;
}
