#ifndef GLOBALS_H
#define GLOBALS_H

//#include <sys/types.h>
#include <string>
#include <vector>
#include <deque>

#include <semaphore.h>
#include <OgreColourValue.h>
#include <OgreSceneManager.h>

class RenderRequest;
class ServerNotification;
class ClientNotification;
class Socket;
class GameMap;
template<typename T> class ProtectedObject;
class ExampleFrameListener;

extern GameMap gameMap;
extern Ogre::SceneManager* mSceneMgr;

extern std::deque<RenderRequest*> renderQueue;
extern sem_t randomGeneratorLockSemaphore;
extern sem_t lightNumberLockSemaphore;
extern sem_t missileObjectUniqueNumberLockSemaphore;
extern sem_t renderQueueSemaphore;
extern sem_t renderQueueEmptySemaphore;
extern ProtectedObject<unsigned> numThreadsWaitingOnRenderQueueEmpty;

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
extern ProtectedObject<long> turnNumber;

extern std::vector<Ogre::ColourValue> playerColourValues;

extern ExampleFrameListener *exampleFrameListener;

#endif

