#ifndef NETWORK_H
#define NETWORK_H

#include <deque>
#include <utility>

// Functions called by pthread_create
void *clientSocketProcessor(void *p);
void *serverSocketProcessor(void *p);
void *serverNotificationProcessor(void *p);
void *clientHandlerThread(void *p);
void *creatureAIThread(void *p);

// Other functions  (these are defined in src/Server.cpp)
string formatCommand(string command, string arguments);
void parseCommand(string command, string &commandName, string &arguments);
ChatMessage *processChatMessage(string arguments);

/*! \brief Server Socket Processor Structure
 *
 * This is a data structure used for passing arguments to the serverSocketProcessor(void *p) defined in src/Server.cpp.
 */
class SSPStruct
{
	public:
		Socket *nSocket;
		ExampleFrameListener *nFrameListener;
};

/*! \brief Client Socket Processor Structure
 *
 * This is a data structure used for passing arguments to the clientSocketProcessor(void *p) defined in src/Client.cpp.
 */
class CSPStruct
{
	public:
		Socket *nSocket;
		ExampleFrameListener *nFrameListener;
};

/*! \brief Client Handler Thread Structure
 *
 * This is a data structure used for passing arguments to the clientHandlerThread(void *p) defined in src/Server.cpp.
 */
class CHTStruct
{
	public:
		Socket *nSocket;
		ExampleFrameListener *nFrameListener;
};

/*! \brief Server Notification Processor Structure
 *
 * This is a data structure used for passing arguments to the serverNotiificationProcessor(void *p) defined in src/Server.cpp.
 */
class SNPStruct
{
	public:
		ExampleFrameListener *nFrameListener;
};

#endif

