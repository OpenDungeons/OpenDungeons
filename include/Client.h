#ifndef CLIENT_H
#define CLIENT_H

#include <deque>
#include <utility>

class CSPStruct
{
	public:
		Socket *nSocket;
		ExampleFrameListener *nFrameListener;
};

void *clientSocketProcessor(void *p);

#endif

