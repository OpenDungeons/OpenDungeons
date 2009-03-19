#include <stdio.h>
#include <string>
#include <sys/time.h>
using namespace std;

#include "Defines.h"
#include "Socket.h"
#include "ExampleFrameListener.h"
#include "Network.h"
#include "ChatMessage.h"

void *serverSocketProcessor(void *p)
{
	Socket *sock = ((SSPStruct*)p)->nSocket;
	Socket *curSock;
	ExampleFrameListener *frameListener = ((SSPStruct*)p)->nFrameListener;

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

		pthread_t *clientThread = new pthread_t;
		CHTStruct *params = new CHTStruct;
		params->nSocket = curSock;
		params->nFrameListener = frameListener;
		pthread_create(clientThread, NULL, clientHandlerThread, (void*)params);
		frameListener->clientHandlerThreads.push_back(clientThread);
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

void *creatureAIThread(void *p)
{
	double timeUntilNextTurn = 1.0/turnsPerSecond;
	Ogre::Timer stopwatch;
	unsigned long int timeTaken;

	while(true)
	{
		//FIXME:  Something should be done to make the clock sleep for a shorter time if the AI is slow.
		//timeUntilNextTurn -= evt.timeSinceLastFrame;

		// Do a turn in the game
		stopwatch.reset();
		gameMap.doTurn();
		turnNumber++;
		timeUntilNextTurn = 1.0/turnsPerSecond;
		timeTaken = stopwatch.getMicroseconds();


		if(1e6 * timeUntilNextTurn - timeTaken > 0)
		{
			cout << "\nCreature AI finished " << 1e6*timeUntilNextTurn - timeTaken << "us early.\n";
			cout.flush();

			usleep(1e6 * timeUntilNextTurn - timeTaken );
		}
		else
		{
			cout << "\nCreature AI finished " << 1e6*timeUntilNextTurn - timeTaken << "us late.\n";
			cout.flush();
		}
	}

	// Return something to make the compiler happy
	return NULL;
}

void *clientHandlerThread(void *p)
{
	Socket *curSock = ((CHTStruct*)p)->nSocket;
	ExampleFrameListener *frameListener = ((CHTStruct*)p)->nFrameListener;

	string clientNick = "UNSET_CLIENT_NICKNAME";
	string clientCommand, arguments;
	string tempString, tempString2;

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
			stringstream tempSS;
			frameListener->chatMessages.push_back(new ChatMessage("SERVER_INFORMATION", "Client connect with version: " + arguments, time(NULL)));

			// Tell the client to give us their nickname and to clear their map
			curSock->send(formatCommand("picknick", ""));

			// Set the nickname that the client sends back, tempString2 is just used
			// to discard the command portion of the respone which will be "setnick"
			curSock->recv(tempString);
			parseCommand(tempString, tempString2, clientNick);
			frameListener->chatMessages.push_back(new ChatMessage("SERVER_INFORMATION", "Client nick is: " + clientNick, time(NULL)));


			curSock->send(formatCommand("newmap", ""));

			// Send over the map tiles from the current game map.
			//TODO: Only send the tiles which the client is supposed to see due to fog of war.
			TileMap_t::iterator itr = gameMap.firstTile();
			while(itr != gameMap.lastTile())
			{
				tempString = "";
				tempSS.str(tempString);
				tempSS << itr->second;
				curSock->send(formatCommand("addtile", tempSS.str()));
				// Throw away the ok response
				curSock->recv(tempString);
				itr++;
			}

			// Send over the actual creatures in use on the current game map
			//TODO: Only send the classes which the client is supposed to see due to fog of war.
			for(int i = 0; i < gameMap.numClassDescriptions(); i++)
			{
				//NOTE: This code is duplicated in writeGameMapToFile defined in src/Functions.cpp
				// Changes to this code should be reflected in that code as well
				Creature *tempCreature = gameMap.getClassDescription(i);

				tempString = "";
				tempSS.str(tempString);

				tempSS << tempCreature->className << "\t" << tempCreature->meshName << "\t";
				tempSS << tempCreature->scale.x << "\t" << tempCreature->scale.y << "\t" << tempCreature->scale.z << "\t";
				tempSS << tempCreature->hp << "\t" << tempCreature->mana << "\t";
				tempSS << tempCreature->sightRadius << "\t" << tempCreature->digRate << "\n";

				curSock->send(formatCommand("addclass", tempSS.str()));
				// Throw away the ok response
				curSock->recv(tempString);
			}

			// Send over the class descriptions in use on the current game map
			//TODO: Only send the classes which the client is supposed to see due to fog of war.
			for(int i = 0; i < gameMap.numCreatures(); i++)
			{
				Creature *tempCreature = gameMap.getCreature(i);

				tempString = "";
				tempSS.str(tempString);

				tempSS << tempCreature;

				curSock->send(formatCommand("addcreature", tempSS.str()));
				// Throw away the ok response
				curSock->recv(tempString);
			}

			// Send tell the client to start this turn
			tempString = "";
			tempSS.str(tempString);
			tempSS << turnNumber;
			curSock->send(formatCommand("newturn", tempSS.str()));
		}

		/*
		else if(clientCommand.compare("setnick") == 0)
		{
			clientNick = arguments;
			frameListener->chatMessages.push_back(new ChatMessage("SERVER_INFORMATION", "Client nick is: " + clientNick, time(NULL)));
		}
		*/

		else if(clientCommand.compare("chat") == 0)
		{
			ChatMessage *newMessage = processChatMessage(arguments);

			// Send the message to all the connected clients
			for(unsigned int i = 0; i < frameListener->clientSockets.size(); i++)
			{
				frameListener->clientSockets[i]->send(formatCommand("chat", newMessage->clientNick + ":" + newMessage->message));
			}

			// Put the message in our own queue
			frameListener->chatMessages.push_back(newMessage);
		}
	}
}

