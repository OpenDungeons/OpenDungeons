#ifndef CLIENT_H
#define CLIENT_H

class CSPStruct
{
	public:
		Socket *nSocket;
		ExampleFrameListener *nFrameListener;
};

void *clientSocketProcessor(void *p);

#endif

