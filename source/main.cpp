#include <deque>

#include "Globals.h"
#include "ODApplication.h"
#include "RenderRequest.h"
#include "Socket.h"
#include "ProtectedObject.h"
#include "Random.h"

//TODO: remove all these globals and put them into better places

ProtectedObject<unsigned int> numThreadsWaitingOnRenderQueueEmpty(0);
ProtectedObject<long int> turnNumber(1);

std::deque<ServerNotification*> serverNotificationQueue;
std::deque<ClientNotification*> clientNotificationQueue;

sem_t lightNumberLockSemaphore;
sem_t missileObjectUniqueNumberLockSemaphore;
sem_t serverNotificationQueueSemaphore;
sem_t clientNotificationQueueSemaphore;
sem_t serverNotificationQueueLockSemaphore;
sem_t clientNotificationQueueLockSemaphore;
sem_t creatureAISemaphore;

Socket* serverSocket = 0;
Socket* clientSocket = 0;

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
int main(int argc, char **argv)
#endif
{
#ifdef WIN32
    // Set up windows sockets
    WSADATA wsaData;

    if(WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
    {
        cerr << "Couldn't not find a usable WinSock DLL.n";
        exit(1);
    }
#endif

    sem_init(&lightNumberLockSemaphore, 0, 1);
    sem_init(&missileObjectUniqueNumberLockSemaphore, 0, 1);
    sem_init(&serverNotificationQueueSemaphore, 0, 0);
    sem_init(&clientNotificationQueueSemaphore, 0, 0);
    sem_init(&serverNotificationQueueLockSemaphore, 0, 1);
    sem_init(&clientNotificationQueueLockSemaphore, 0, 1);
    sem_init(&creatureAISemaphore, 0, 1);

    Random::initialize();
    try
    {
        new ODApplication;
    }
    catch (Ogre::Exception& e)
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 
        MessageBox(0, e.what(), "An exception has occurred!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
        fprintf(stderr, "An exception has occurred: %s\n", e.what());
#endif
    }

#ifdef WIN32
    WSACleanup();
#endif

    return 0;
}

