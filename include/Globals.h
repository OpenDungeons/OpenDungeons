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
#include <OgreSceneManager.h>

//Various text strings and constants
const unsigned int PORT_NUMBER = 31222;
const double BLENDER_UNITS_PER_OGRE_UNIT = 10.0;
const double DEFAULT_FRAMES_PER_SECOND = 60.0;
const std::string VERSION = "0.4.7";
const std::string POINTER_INFO_STRING = "pointerInfo";
const std::string HELP_MESSAGE = "\
The console is a way of interacting with the underlying game engine directly.\
Commands given to the the console are made up of two parts: a \'command name\' and one or more \'arguments\'.\
For information on how to use a particular command, type help followed by the command name.\
\n\nThe following commands are avaliable:\
\n\thelp keys - shows the keyboard controls\
\n\tlist - print out lists of creatures, classes, etc\n\thelp - displays this help screen\n\tsave - saves the current level to a file\
\n\tload - loads a level from a file\
\n\tquit - exit the program\
\n\tmaxmessages - Sets or displays the max number of chat messages to display\
\n\tmaxtime - Sets or displays the max time for chat messages to be displayed\
\n\ttermwidth - set the terminal width\
\n\taddcreature - load a creature into the file.\
\n\taddclass - Define a creature class\
\n\taddtiles - adds a rectangular region of tiles\
\n\tnewmap - Creates a new rectangular map\
\n\trefreshmesh - Reloads the meshes for all the objects in the game\
\n\tmovespeed - sets the camera movement speed\
\n\trotatespeed - sets the camera rotation speed\
\n\tfps - sets the maximum framerate\
\n\tturnspersecond - sets the number of turns the AI will carry out per second\
\n\tmousespeed - sets the mouse speed\
\n\tambientlight - set the ambient light color\
\n\tconnect - connect to a server\
\n\thost - host a server\
\n\tchat - send a message to other people in the game\
\n\tnearclip - sets the near clipping distance\
\n\tfarclip - sets the far clipping distance\
\n\tvisdebug - turns on visual debugging for a creature\
\n\taddcolor - adds another player color\
\n\tsetcolor - changes the value of one of the player's color\
\n\tdisconnect - stops a running server or client and returns to the map editor\
\n\taithreads - sets the maximum number of creature AI threads on the server";

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

extern Socket* serverSocket;
extern Socket* clientSocket;

extern std::string versionString;
extern std::string MOTD;
extern double MAX_FRAMES_PER_SECOND;
extern double turnsPerSecond;
extern ProtectedObject<long> turnNumber;

extern std::vector<Ogre::ColourValue> playerColourValues;

extern ExampleFrameListener* exampleFrameListener;

#endif
