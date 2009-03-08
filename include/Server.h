
#ifndef SERVER_H
#define SERVER_H

#include <deque>
#include <utility>

class SSPStruct
{
	public:
		Socket *nSocket;
		ExampleFrameListener *nFrameListener;
};

void *serverSocketProcessor(void *p);

#endif

