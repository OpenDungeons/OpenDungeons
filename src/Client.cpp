#include "stdio.h"
#include <string>
using namespace std;

#include "Defines.h"
#include "Socket.h"
#include "ExampleFrameListener.h"
#include "Network.h"
#include "ChatMessage.h"

void *clientSocketProcessor(void *p)
{
	string tempString;
	string serverCommand, arguments;
	Socket *sock = ((CSPStruct*)p)->nSocket;
	ExampleFrameListener *frameListener = ((CSPStruct*)p)->nFrameListener;


	// Send a hello request to start the conversation with the server
	sock->send(formatCommand("hello", (string)"OpenDungeons V " + VERSION));
	while(sock->is_valid())
	{
		int charsRead = sock->recv(tempString);
		// If the server closed the connection
		if(charsRead <= 0)
		{
			break;
		}

		parseCommand(tempString, serverCommand, arguments);

		if(serverCommand.compare("picknick") == 0)
		{
			sock->send(formatCommand("setnick", me->nick));
		}

		else if(serverCommand.compare("chat") == 0)
		{
			ChatMessage *newMessage = processChatMessage(arguments);
			frameListener->chatMessages.push_back(newMessage);
		}

		else if(serverCommand.compare("newmap") == 0)
		{
			gameMap.clearAll();
		}
	}
}

