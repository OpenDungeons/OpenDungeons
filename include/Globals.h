#ifndef GLOBALS_H
#define GLOBALS_H

#include <sys/types.h>
#include <semaphore.h>

#include "Tile.h"
#include "Player.h"
#include "GameMap.h"
#include "RenderRequest.h"
#include "ServerNotification.h"
#include "ClientNotification.h"
#include "Socket.h"

extern GameMap gameMap;
extern SceneManager* mSceneMgr;

extern deque<RenderRequest*> renderQueue;
extern sem_t renderQueueSemaphore;

extern deque<ServerNotification*> serverNotificationQueue;
extern deque<ClientNotification*> clientNotificationQueue;
extern sem_t serverNotificationQueueSemaphore;
extern sem_t clientNotificationQueueSemaphore;
extern sem_t serverNotificationQueueLockSemaphore;
extern sem_t clientNotificationQueueLockSemaphore;

extern Socket *serverSocket, *clientSocket;

extern string MOTD;
extern double MAX_FRAMES_PER_SECOND;
extern double turnsPerSecond;
extern long int turnNumber;

#endif

