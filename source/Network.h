#ifndef NETWORK_H
#define NETWORK_H

#include <deque>
#include <string>
#include <utility>

class ODFrameListener;
class Socket;

#include "ChatMessage.h"

/*! \brief A thread function which runs on the server and listens for new connections from clients.
 *
 * A single instance of this thread is spawned by running the "host" command
 * from the in-game console.  The thread then binds annd listens on the
 * specified port and when clients connect a new socket, and a
 * clientHandlerThread are spawned to handle communications with that client.
 * This function currently has no way of breaking out of its primary loop, so
 * once it is started it never exits until the program is closed.
 */
void processServerSocketMessages();

/*! \brief Monitors the serverNotificationQueue for new events and informs the clients about them.
 *
 * This function is used in server mode and acts as a "consumer" on the
 * serverNotificationQueue.  It takes an event out of the queue, determines
 * which clients need to be informed about that particular event, and
 * dispacthes TCP packets to inform the clients about the new information.
 */
void processServerNotifications();

void *clientHandlerThread(void *p);

// Functions called by pthread_create which run on the client
void *clientSocketProcessor(void *p);
void *clientNotificationProcessor(void *p);

// Other functions  (these are defined in src/Server.cpp)
std::string formatCommand(std::string command, std::string arguments);
bool parseCommand(std::string &command, std::string &commandName, std::string &arguments);
ChatMessage *processChatMessage(std::string arguments);
void sendToAllClients(ODFrameListener *frameListener, std::string str);

/*! \brief Server Socket Processor Structure
 *
 * This is a data structure used for passing arguments to the serverSocketProcessor(void *p) defined in src/Server.cpp.
 */
class SSPStruct
{
    public:
        Socket *nSocket;
        ODFrameListener *nFrameListener;
};

/*! \brief Client Socket Processor Structure
 *
 * This is a data structure used for passing arguments to the clientSocketProcessor(void *p) defined in src/Client.cpp.
 */
class CSPStruct
{
    public:
        Socket *nSocket;
        ODFrameListener *nFrameListener;
};

/*! \brief Client Handler Thread Structure
 *
 * This is a data structure used for passing arguments to the clientHandlerThread(void *p) defined in src/Server.cpp.
 */
class CHTStruct
{
    public:
        Socket *nSocket;
        ODFrameListener *nFrameListener;
};

/*! \brief Server Notification Processor Structure
 *
 * This is a data structure used for passing arguments to the serverNotificationProcessor(void *p) defined in src/Server.cpp.
 */
class SNPStruct
{
    public:
        ODFrameListener *nFrameListener;
};

/*! \brief Client Notification Processor Structure
 *
 * This is a data structure used for passing arguments to the clientNotificationProcessor(void *p) defined in src/Client.cpp.
 */
class CNPStruct
{
    public:
        ODFrameListener *nFrameListener;
};

#endif

