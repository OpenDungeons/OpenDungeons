#include <string>
using namespace std;

#include "Defines.h"
#include "Socket.h"
#include "ExampleFrameListener.h"
#include "Client.h"

void *clientSocketProcessor(void *p)
{
	string tempString;
	Socket *sock = ((CSPStruct*)p)->nSocket;
	ExampleFrameListener *frameListener = ((CSPStruct*)p)->nFrameListener;

	sock->send((string)"OpenDungeons V" + VERSION + "\n");
	while(sock->is_valid())
	{
		sock->recv(tempString);
		frameListener->chatString += tempString;
	}
}

