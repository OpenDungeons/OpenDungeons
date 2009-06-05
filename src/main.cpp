#include <vector>
#include <deque>
#include <semaphore.h>
using namespace std;

#include "Defines.h"
#include "Functions.h"
#include "MapEditor.h"
#include "GameMap.h"
#include "Player.h"
#include "RenderRequest.h"
#include "Socket.h"

SceneManager* mSceneMgr;
GameMap gameMap;
string MOTD = (string)"Welcome to Open Dungeons\tVersion:  " + VERSION;
double MAX_FRAMES_PER_SECOND = DEFAULT_FRAMES_PER_SECOND;
vector<Player*> players;
Player *me;
double turnsPerSecond = 1.0;
long int turnNumber = 1;
deque<RenderRequest*> renderQueue;
sem_t renderQueueSemaphore;
deque<ServerNotification*> serverNotificationQueue;
sem_t serverNotificationQueueSemaphore;
Socket *serverSocket = NULL, *clientSocket = NULL;

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
int main(int argc, char **argv)
#endif
{
	seedRandomNumberGenerator();
	sem_init(&renderQueueSemaphore, 0, 1);
	sem_init(&serverNotificationQueueSemaphore, 0, 0);

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

	return 0;
}

