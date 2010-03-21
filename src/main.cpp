#include <vector>
#include <deque>
#include <semaphore.h>
#include <iostream>
using namespace std;

#include "Defines.h"
#include "Functions.h"
#include "MapEditor.h"
#include "GameMap.h"
#include "Player.h"
#include "RenderRequest.h"
#include "Socket.h"
#include "ProtectedObject.h"

SceneManager* mSceneMgr;
GameMap gameMap;

deque<RenderRequest*> renderQueue;
sem_t renderQueueSemaphore;
sem_t renderQueueEmptySemaphore;
ProtectedObject<unsigned int> numThreadsWaitingOnRenderQueueEmpty(0);

deque<ServerNotification*> serverNotificationQueue;
deque<ClientNotification*> clientNotificationQueue;
sem_t serverNotificationQueueSemaphore;
sem_t clientNotificationQueueSemaphore;
sem_t serverNotificationQueueLockSemaphore;
sem_t clientNotificationQueueLockSemaphore;

sem_t creatureAISemaphore;

Socket *serverSocket = NULL, *clientSocket = NULL;

string versionString = (string)"OpenDungeons_Version:" + VERSION;
string MOTD = (string)"Welcome to Open Dungeons\tVersion:  " + VERSION;
double MAX_FRAMES_PER_SECOND = DEFAULT_FRAMES_PER_SECOND;
double turnsPerSecond = 1.0;
ProtectedObject<long int> turnNumber(1);

vector<ColourValue> playerColourValues;

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
int main(int argc, char **argv)
#endif
{
	// Set up windows sockets
#ifdef WIN32
	WSADATA wsaData;
	WORD wVersionRequested;
	int err;

	wVersionRequested = MAKEWORD( 2, 0 ); // 2.0 and above version of WinSock
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 )
	{
		cerr << "Couldn't not find a usable WinSock DLL.n";
		exit(1);
	}
#endif

	seedRandomNumberGenerator();
	sem_init(&renderQueueSemaphore, 0, 1);
	sem_init(&renderQueueEmptySemaphore, 0, 0);
	sem_init(&serverNotificationQueueSemaphore, 0, 0);
	sem_init(&clientNotificationQueueSemaphore, 0, 0);
	sem_init(&serverNotificationQueueLockSemaphore, 0, 1);
	sem_init(&clientNotificationQueueLockSemaphore, 0, 1);
	sem_init(&creatureAISemaphore, 0, 1);

	playerColourValues.push_back(ColourValue(0.8, 0.8, 0.8, 1.0));
	playerColourValues.push_back(ColourValue(0.8, 0.0, 0.0, 1.0));
	playerColourValues.push_back(ColourValue(0.0, 0.8, 0.0, 1.0));
	playerColourValues.push_back(ColourValue(0.0, 0.0, 0.8, 1.0));
	playerColourValues.push_back(ColourValue(0.4, 0.4, 0.4, 1.0));
	playerColourValues.push_back(ColourValue(0.4, 0.0, 0.0, 1.0));
	playerColourValues.push_back(ColourValue(0.0, 0.4, 0.0, 1.0));
	playerColourValues.push_back(ColourValue(0.0, 0.0, 0.4, 1.0));

	// Create application object
	MapEditor app;

	try {
	app.go();
	} catch( Exception& e ) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 
	MessageBox( NULL, e.what(), "An exception has occurred!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
	fprintf(stderr, "An exception has occurred: %s\n",
	e.what());
#endif
	}

#ifdef WIN32
	WSACleanup();
#endif


	return 0;
}

