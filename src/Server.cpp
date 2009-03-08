#include <stdio.h>
#include <string>
using namespace std;

#include "Defines.h"
#include "Socket.h"
#include "ExampleFrameListener.h"
#include "Network.h"
#include "ChatMessage.h"

void *serverSocketProcessor(void *p)
{
	string clientCommand, arguments;
	string tempString;
	Socket *sock = ((SSPStruct*)p)->nSocket;
	Socket *curSock;
	ExampleFrameListener *frameListener = ((SSPStruct*)p)->nFrameListener;
	string clientNick = "UNSET_CLIENT_NICKNAME";

	// Set up the socket to listen on the specified port
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

	// Listen for connectections and spawn a new socket+thread to handle them
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
			// Recieve a request from the client and store it in tempString
			int charsRead = curSock->recv(tempString);

			// If the client closed the connection
			if(charsRead <= 0)
			{
				break;
			}

			// If this command is not seperated by a colon into a
			// command and an argument then don't process it.  Send
			// the client an error message and move on to the next packet.
			unsigned int index = tempString.find(":");
			if(index == string::npos)
			{
				continue;
			}

			// Split the packet into a command and an argument
			parseCommand(tempString, clientCommand, arguments);
			//clientCommand = tempString.substr(1, index-1);
			//arguments = tempString.substr(index+1, tempString.size()-index-3);
			cout << "\n\n\n" << clientCommand << "\n" << arguments;
			cout.flush();

			if(clientCommand.compare("hello") == 0)
			{
				ChatMessage *newMessage = new ChatMessage("SERVER_INFORMATION", arguments, time(NULL));
				frameListener->chatMessages.push_back(newMessage);

				int charsSent = curSock->send(formatCommand("picknick", ""));
			}

			else if(clientCommand.compare("setnick") == 0)
			{
				clientNick = arguments;
			}

			else if(clientCommand.compare("chat") == 0)
			{
				ChatMessage *newMessage = processChatMessage(arguments);
				frameListener->chatMessages.push_back(newMessage);
			}
		}
	}
}

string formatCommand(string command, string arguments)
{
	return "<" + command + ":" + arguments + ">";
}

void parseCommand(string command, string &commandName, string &arguments)
{
	int index, index2;
	index = command.find("<");
	index2 = command.find(":");
	commandName = command.substr(index+1, index2-1);
	index = index2;
	index2 = command.find(">");
	arguments = command.substr(index+1, index2-index-1);
	cout << "\n\n\nParse command:  " << command << "\n" << commandName << "\n" << arguments << "\n\n";
}

ChatMessage *processChatMessage(string arguments)
{
	int index = arguments.find(":");
	string messageNick = arguments.substr(0, index);
	string message = arguments.substr(index+1, arguments.size()-index-1);

	return new ChatMessage(messageNick,message,time(NULL));
}

