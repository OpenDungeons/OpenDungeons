#include "stdio.h"
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
		int charsRead = sock->recv(tempString);
		// If the server closed the connection
		if(charsRead <= 0)
		{
			break;
		}

		frameListener->chatMessages.push_back(pair<time_t,string>(time(NULL),tempString));
	}
}

