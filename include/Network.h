#ifndef NETWORK_H
#define NETWORK_H

#include <deque>
#include <utility>

// Functions called by pthread_create
void *clientSocketProcessor(void *p);
void *serverSocketProcessor(void *p);

// Other functions  (these are defined in src/Server.cpp)
string formatCommand(string command, string arguments);
void parseCommand(string command, string &commandName, string &arguments);
ChatMessage *processChatMessage(string arguments);

// A pair of structures to pass parameters to the thread functions listed above.
class SSPStruct
{
	public:
		Socket *nSocket;
		ExampleFrameListener *nFrameListener;
};

class CSPStruct
{
	public:
		Socket *nSocket;
		ExampleFrameListener *nFrameListener;
};

#endif

