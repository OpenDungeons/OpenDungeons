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
#include "ProtectedObject.h"
#include "ExampleFrameListener.h"

extern GameMap gameMap;
extern SceneManager* mSceneMgr;

extern std::deque<RenderRequest*> renderQueue;
extern sem_t randomGeneratorLockSemaphore;
extern sem_t renderQueueSemaphore;
extern sem_t renderQueueEmptySemaphore;
extern ProtectedObject<unsigned int> numThreadsWaitingOnRenderQueueEmpty;

extern std::deque<ServerNotification*> serverNotificationQueue;
extern std::deque<ClientNotification*> clientNotificationQueue;
extern sem_t serverNotificationQueueSemaphore;
extern sem_t clientNotificationQueueSemaphore;
extern sem_t serverNotificationQueueLockSemaphore;
extern sem_t clientNotificationQueueLockSemaphore;

extern sem_t creatureAISemaphore;

extern Socket *serverSocket, *clientSocket;

extern std::string versionString;
extern std::string MOTD;
extern double MAX_FRAMES_PER_SECOND;
extern double turnsPerSecond;
extern ProtectedObject<long int> turnNumber;

extern std::vector<ColourValue> playerColourValues;

extern ExampleFrameListener *exampleFrameListener;

#endif

