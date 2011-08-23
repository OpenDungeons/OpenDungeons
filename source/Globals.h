//TODO: get rid of this whole file. globals/externals are bad.
//      put everything in good (singleton) classes or pass as parameter
//      or at least use good namespaces (encapsulate privates by using
//      unnamed namespaced)
#ifndef GLOBALS_H
#define GLOBALS_H

#ifdef __MINGW32__
#ifndef mode_t
#include <sys/types.h>
#endif //mode_t
#endif //mingw32

#include <string>
#include <vector>
#include <deque>

#include <semaphore.h>
#include <OgreColourValue.h>

class RenderRequest;
class ServerNotification;
class ClientNotification;
class Socket;
class GameMap;
template<typename T> class ProtectedObject;

//extern GameMap gameMap;

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

extern Socket* serverSocket;
extern Socket* clientSocket;

extern ProtectedObject<long> turnNumber;

//extern std::vector<Ogre::ColourValue> playerColourValues;

#endif
