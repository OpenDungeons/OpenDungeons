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

		else if(serverCommand.compare("addtile") == 0)
		{
			stringstream tempSS(arguments);
			Tile *newTile = new Tile;
			tempSS >> newTile;
			gameMap.addTile(newTile);
			//newTile->createMesh();
			sock->send(formatCommand("ok", "addtile"));
		}

		else if(serverCommand.compare("addclass") == 0)
		{
			// This code is duplicated in readGameMapFromFile defined in src/Functions.cpp
			// Changes to this code should be reflected in that code as well
			double tempX, tempY, tempZ, tempSightRadius, tempDigRate;
			int tempHP, tempMana;
			stringstream tempSS;
			string tempString2;

			tempSS.str(arguments);

			tempSS >> tempString >> tempString2 >> tempX >> tempY >> tempZ;
			tempSS >> tempHP >> tempMana;
			tempSS >> tempSightRadius >> tempDigRate;

			Creature *p = new Creature(tempString, tempString2, Ogre::Vector3(tempX, tempY, tempZ), tempHP, tempMana, tempSightRadius, tempDigRate);
			gameMap.addClassDescription(p);
			sock->send(formatCommand("ok", "addclass"));
		}

		else
		{
			cout << "\n\n\nERROR:  Unknown server command!\nCommand:";
			cout << serverCommand << "\nArguments:" << arguments << "\n\n";
		}
	}


}

