#include "stdio.h"
#include <string>
using namespace std;

#include "Defines.h"
#include "Socket.h"
#include "ExampleFrameListener.h"
#include "Server.h"

void *serverSocketProcessor(void *p)
{
	string tempString;
	Socket *sock = ((SSPStruct*)p)->nSocket;
	Socket *curSock;
	ExampleFrameListener *frameListener = ((SSPStruct*)p)->nFrameListener;

	if(!sock->create())
	{
		frameListener->commandOutput = "ERROR:  Server could not create server socket!";
		return NULL;
	}

	int port = PORT_NUMBER;
	if(!sock->bind(port))
	{
		frameListener->commandOutput += "ERROR:  Server could not bind to port!";
		return NULL;
	}

	while(true)
	{
		// Wait until a client connects
		if(!sock->listen())
		{
			frameListener->commandOutput += "ERROR:  Server could not listen!";
			return NULL;
		}

		// create a new socket to handle the connection with this client
		curSock = new Socket;
		sock->accept(*curSock);

		//FIXME:  Also need to remove this pointer from the vector when the connection closes.
		frameListener->clientSockets.push_back(curSock);

		while(true)
		{
			int charsRead = curSock->recv(tempString);
			// If the server closed the connection
			if(charsRead <= 0)
			{
				break;
			}

			frameListener->chatMessages.push_back(pair<time_t,string>(time(NULL),tempString));
		}
	}
}

