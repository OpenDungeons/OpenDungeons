#include <string>
#include <ctime>

#include "Socket.h"
#include "ODFrameListener.h"
#include "Network.h"
#include "ChatMessage.h"
#include "ODServer.h"
#include "ServerNotification.h"
#include "Player.h"
#include "Tile.h"
#include "MapLight.h"
#include "GameMap.h"
#include "ProtectedObject.h"
#include "Creature.h"
#include "ODApplication.h"
#include "LogManager.h"
#include "Seat.h"
#include "GameMap.h"

void processServerSocketMessages()
{
    Socket* sock = Socket::serverSocket;

    // Not a server
    if (!sock)
        return;

    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    GameMap* gameMap = frameListener->getGameMap();

    // Listen for connections and spawn a new socket+thread to handle them

    // Wait until a client connects
    if (!sock->listen())
    {
        frameListener->setConsoleCommandOutput("ERROR:  Server could not listen!");
        return;
    }

    // FIXME: We return now as the socket only handles the dedicated server listening for now.
    return;

    // create a new socket to handle the connection with this client
    Socket* curSock = new Socket;
    if (!sock->accept(*curSock))
    {
        delete curSock;
        return;
    }

    //FIXME:  Also need to remove this pointer from the vector when the connection closes.
    frameListener->mClientSockets.push_back(curSock);
}

/*! \brief A helper function to pack a message into a packet to send over the network.
 *
 * This function packs the command and arguments to be sent over the network
 * into a return string which is then sent over the network.  This decouples
 * the encoding from the actual program code so changes in the wire protocol
 * are confined to this function and its sister function, parseCommand.
 */
std::string formatCommand(std::string command, std::string arguments)
{
    //FIXME:  Need to protect the ":" symbol with an escape sequence.
    return "<" + command + ":" + arguments + ">";
}

/*! \brief A helper function to unpack a message from a packet received over the network.
 *
 * This function unpacks the command and arguments sent over the network into
 * two strings.  This decouples the decoding from the actual program code so
 * changes in the wire protocol are confined to this function and its sister
 * function, formatCommand.
 */
bool parseCommand(std::string &command, std::string &commandName, std::string &arguments)
{
    //FIXME:  Need to protect the ":" symbol with an escape sequence.
    int index, index2;
    index = command.find("<");
    index2 = command.find(":");
    commandName = command.substr(index + 1, index2 - 1);
    index = index2;
    index2 = command.find(">");
    arguments = command.substr(index + 1, index2 - index - 1);
    command = command.substr(index2 + 1, command.length() - index2 + 1);
    //cout << "\n\n\nParse command:  " << command << "\n" << commandName << "\n" << arguments << "\n\n";

    return (command.length() > 0);
}

/*! \brief A helper function to unpack the argument of a chat command into a ChatMessage structure.
 *
 * Once a command recieved from the network and hase been parsed by
 * parseCommand, this function then takes the argument of that message and
 * further unpacks a username and a chat message.
 */
ChatMessage* processChatMessage(std::string arguments)
{
    int index = arguments.find(":");
    std::string messageNick = arguments.substr(0, index);
    std::string message = arguments.substr(index + 1, arguments.size() - index - 1);

    return new ChatMessage(messageNick, message, time(NULL));
}

void processServerNotifications()
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    GameMap* gameMap = frameListener->getGameMap();

    std::stringstream tempSS;
    Tile *tempTile;
    Player *tempPlayer;
    MapLight *tempMapLight;
    MovableGameEntity *tempAnimatedObject;
    bool running = true;

    while (running)
    {
        // If the queue is empty, let's get out of the loop.
        if (ServerNotification::serverNotificationQueue.empty())
            break;

        // Take a message out of the front of the notification queue
        ServerNotification *event = ServerNotification::serverNotificationQueue.front();
        ServerNotification::serverNotificationQueue.pop_front();

        //FIXME:  This really should never happen but the queue does occasionally pop a NULL.
        //This is probably a bug somewhere else where a NULL is being place in the queue.
        if (event == NULL)
            continue;

        switch (event->type)
        {
            case ServerNotification::turnStarted:
                tempSS.str("");
                tempSS << GameMap::turnNumber.get();

                sendToAllClients(frameListener, formatCommand("newturn", tempSS.str()));
                break;

            case ServerNotification::animatedObjectAddDestination:
                tempSS.str("");
                tempSS << event->str << ":" << event->vec.x << ":"
                        << event->vec.y << ":" << event->vec.z;

                sendToAllClients(frameListener, formatCommand(
                        "animatedObjectAddDestination", tempSS.str()));
                break;

            case ServerNotification::animatedObjectClearDestinations:
                tempSS.str("");
                tempSS << event->ani->getName();
                sendToAllClients(frameListener, formatCommand(
                        "animatedObjectClearDestinations", tempSS.str()));
                break;

                //NOTE: this code is duplicated in clientNotificationProcessor
            case ServerNotification::creaturePickUp:
                tempSS.str("");
                tempSS << event->player->getNick() << ":" << event->cre->getName();

                sendToAllClients(frameListener, formatCommand("creaturePickUp",
                        tempSS.str()));
                break;

                //NOTE: this code is duplicated in clientNotificationProcessor
            case ServerNotification::creatureDrop:
                tempPlayer = event->player;
                tempTile = event->tile;

                tempSS.str("");
                tempSS << tempPlayer->getNick() << ":" << tempTile->x << ":"
                        << tempTile->y;

                sendToAllClients(frameListener, formatCommand("creatureDrop",
                        tempSS.str()));
                break;

            case ServerNotification::setObjectAnimationState:
                tempSS.str("");
                tempAnimatedObject = static_cast<MovableGameEntity*>(event->p);
                tempSS << tempAnimatedObject->getName() << ":" << event->str
                        << ":" << (event->b ? "true" : "false");
                sendToAllClients(frameListener, formatCommand(
                        "setObjectAnimationState", tempSS.str()));
                break;

            case ServerNotification::setTurnsPerSecond:
                tempSS.str("");
                tempSS << ODApplication::turnsPerSecond;

                sendToAllClients(frameListener, formatCommand("turnsPerSecond",
                        tempSS.str()));
                break;

            case ServerNotification::tileFullnessChange:
                tempSS.str("");
                tempTile = event->tile;
                tempSS << tempTile->getFullness() << ":" << tempTile->x << ":"
                        << tempTile->y;

                sendToAllClients(frameListener, formatCommand(
                        "tileFullnessChange", tempSS.str()));
                break;

            case ServerNotification::addMapLight:
                tempMapLight = static_cast<MapLight*>(event->p);
                tempSS.str("");
                tempSS << tempMapLight;
                sendToAllClients(frameListener, formatCommand("addmaplight",
                        tempSS.str()));
                break;

            case ServerNotification::removeMapLight:
                tempMapLight = static_cast<MapLight*>(event->p);
                sendToAllClients(frameListener, formatCommand("removeMapLight",
                        tempMapLight->getName()));
                break;

            case ServerNotification::exit:
            {
                running = false;
                break;
            }

            default:
                std::cout << "\n\nERROR:  Unhandled ServerNotification type encoutered!\n\n";
                break;
        }

        // Decrement the number of outstanding references to things from the turn number the event was queued on.
        gameMap->threadUnlockForTurn(event->turnNumber);

        delete event;
        event = NULL;
    }
}

bool processClientNotifications(Socket* clientSocket)
{
    if (!clientSocket)
        return false;

    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    GameMap* gameMap = frameListener->getGameMap();
    if (!gameMap)
        return true;

    Player *curPlayer = NULL;

    std::string clientNick = "UNSET_CLIENT_NICKNAME";
    std::string clientCommand, arguments;
    std::string tempString, tempString2;

    // Recieve a request from the client and store it in tempString
    int charsRead = clientSocket->recv(tempString);

    // If the client closed the connection
    // FIXME: Hint the client socker container to delete this reference.
    if (charsRead <= 0)
    {
        frameListener->addChatMessage(new ChatMessage("SERVER_INFORMATION: ", "Client disconnect: "
                                                      + clientNick, time(NULL)));
        return false;
    }

    // If this command is not seperated by a colon into a
    // command and an argument then don't process it.
    // TODO: Send the client an error message.
    unsigned int index = tempString.find(":");
    if (index == std::string::npos)
        return true;

    // Split the packet into a command and an argument
    parseCommand(tempString, clientCommand, arguments);
    //clientCommand = tempString.substr(1, index-1);
    //arguments = tempString.substr(index+1, tempString.size()-index-3);
    //cout << "\n\n\n" << clientCommand << "\n" << arguments;
    //cout.flush();

    if (clientCommand.compare("hello") == 0)
    {
        std::stringstream tempSS;
        frameListener->addChatMessage(new ChatMessage(
                "SERVER_INFORMATION: ", "Client connect with version: "
                        + arguments, time(NULL)));

        // Tell the client to give us their nickname and to clear their map
        clientSocket->send(formatCommand("picknick", ""));

        // Set the nickname that the client sends back, tempString2 is just used
        // to discard the command portion of the respone which should be "setnick"
        //TODO:  verify that this really is true
        clientSocket->recv(tempString);
        parseCommand(tempString, tempString2, clientNick);
        frameListener->addChatMessage(new ChatMessage(
                "SERVER_INFORMATION: ", "Client nick is: " + clientNick,
                time(NULL)));

        // Create a player structure for the client
        //TODO:  negotiate and set a color
        curPlayer = new Player;
        curPlayer->setNick(clientNick);
        gameMap->addPlayer(curPlayer);

        clientSocket->send(formatCommand("newmap", ""));

        // Tell the player which seat it has
        tempSS.str("");
        tempSS << curPlayer->getSeat();
        clientSocket->send(formatCommand("addseat", tempSS.str()));

        tempSS.str("");
        tempSS << ODApplication::turnsPerSecond;
        clientSocket->send(formatCommand("turnsPerSecond", tempSS.str()));

        // Send over the information about the players in the game
        clientSocket->send(formatCommand("addplayer", gameMap->getLocalPlayer()->getNick()));
        for (unsigned int i = 0; i < gameMap->numPlayers(); ++i)
        {
            // Don't tell the client about its own player structure
            Player* tempPlayer = gameMap->getPlayer(i);
            if (curPlayer != tempPlayer && tempPlayer != NULL)
            {
                tempSS.str("");
                tempSS << tempPlayer->getSeat();
                clientSocket->send(formatCommand("addseat", tempSS.str()));
                // Throw away the ok response
                clientSocket->recv(tempString);

                clientSocket->send(formatCommand("addplayer", tempPlayer->getNick()));
                // Throw away the ok response
                clientSocket->recv(tempString);
            }
        }

        // Send over the map tiles from the current game map.
        //TODO: Only send the tiles which the client is supposed to see due to fog of war.
        for(int ii = 0; ii < gameMap->getMapSizeX(); ++ii)
        {
            for(int jj = 0; jj < gameMap->getMapSizeY(); ++jj)
            {
                tempSS.str("");
                tempSS << gameMap->getTile(ii,jj);
                clientSocket->send(formatCommand("addtile", tempSS.str()));
                // Throw away the ok response
                clientSocket->recv(tempString);
            }
        }

        // Send over the map lights from the current game map.
        //TODO: Only send the maplights which the client is supposed to see due to the fog of war.
        for (unsigned int ii = 0; ii < gameMap->numMapLights(); ++ii)
        {
            tempSS.str("");
            tempSS << gameMap->getMapLight(ii);
            clientSocket->send(formatCommand("addmaplight", tempSS.str()));

        }

        // Send over the rooms in use on the current game map
        //TODO: Only send the classes which the client is supposed to see due to fog of war.
        for (unsigned int ii = 0; ii < gameMap->numRooms(); ++ii)
        {
            tempSS.str("");
            tempSS << gameMap->getRoom(ii);
            clientSocket->send(formatCommand("addroom", tempSS.str()));
            // Throw away the ok response
            clientSocket->recv(tempString);
        }

        // Send over the class descriptions in use on the current game map
        //TODO: Only send the classes which the client is supposed to see due to fog of war.
        for (unsigned int i = 0; i < gameMap->numClassDescriptions(); ++i)
        {
            //NOTE: This code is duplicated in writeGameMapToFile defined in src/Functions.cpp
            // Changes to this code should be reflected in that code as well
            CreatureDefinition *tempClass = gameMap->getClassDescription(i);

            tempSS.str("");

            tempSS << tempClass;

            clientSocket->send(formatCommand("addclass", tempSS.str()));
            // Throw away the ok response
            //TODO:  Actually check this.
            clientSocket->recv(tempString);
        }

        // Send over the actual creatures in use on the current game map
        //TODO: Only send the creatures which the client is supposed to see due to fog of war.
        for (unsigned int i = 0; i < gameMap->numCreatures(); ++i)
        {
            Creature *tempCreature = gameMap->getCreature(i);

            tempSS.str("");

            tempSS << tempCreature;

            clientSocket->send(formatCommand("addcreature", tempSS.str()));
            // Throw away the ok response
            clientSocket->recv(tempString);
        }
    }

    else if (clientCommand.compare("chat") == 0)
    {
        ChatMessage *newMessage = processChatMessage(arguments);

        // Send the message to all the connected clients
        for (unsigned int i = 0; i < frameListener->mClientSockets.size(); ++i)
        {
            frameListener->mClientSockets[i]->send(formatCommand("chat",
                    newMessage->getClientNick() + ":" + newMessage->getMessage()));
        }

        // Put the message in our own queue
        frameListener->addChatMessage(newMessage);
    }

    //NOTE:  This code is duplicated in clientSocketProcessor()
    else if (clientCommand.compare("creaturePickUp") == 0)
    {
        char array[255];

        std::stringstream tempSS;
        tempSS.str(arguments);

        tempSS.getline(array, sizeof(array), ':');
        std::string playerNick = array;
        tempSS.getline(array, sizeof(array));
        std::string creatureName = array;

        Player *tempPlayer = gameMap->getPlayer(playerNick);
        Creature *tempCreature = gameMap->getCreature(creatureName);

        if (tempPlayer != NULL && tempCreature != NULL)
        {
            tempPlayer->pickUpCreature(tempCreature);
        }
    }

    //NOTE:  This code is duplicated in clientSocketProcessor()
    else if (clientCommand.compare("creatureDrop") == 0)
    {
        char array[255];

        std::stringstream tempSS;
        tempSS.str(arguments);

        tempSS.getline(array, sizeof(array), ':');
        std::string playerNick = array;
        tempSS.getline(array, sizeof(array), ':');
        int tempX = atoi(array);
        tempSS.getline(array, sizeof(array));
        int tempY = atoi(array);

        Player *tempPlayer = gameMap->getPlayer(playerNick);
        Tile *tempTile = gameMap->getTile(tempX, tempY);

        if (tempPlayer != NULL && tempTile != NULL)
        {
            tempPlayer->dropCreature(tempTile);
        }
    }

    else if (clientCommand.compare("markTile") == 0)
    {
        char array[255];
        std::stringstream tempSS;
        tempSS.str(arguments);

        tempSS.getline(array, sizeof(array), ':');
        int tempX = atoi(array);
        tempSS.getline(array, sizeof(array), ':');
        int tempY = atoi(array);
        tempSS.getline(array, sizeof(array));
        std::string flagName = array;

        Tile *tempTile = gameMap->getTile(tempX, tempY);
        if (tempTile != NULL)
        {
            Player *tempPlayer = gameMap->getPlayer(clientNick);
            if (tempPlayer != NULL)
            {
                bool flag;
                flagName.compare("true") == 0 ? flag = true : flag = false;
                tempTile->setMarkedForDigging(flag, tempPlayer);
            }
        }
    }

    else if (clientCommand.compare("ok") == 0)
    {
        std::cout << "\nIgnoring an ak message from a client: " << arguments;
    }

    else
    {
        std::cerr << "\n\nERROR:  Unhandled command recieved from client:\nCommand:  ";
        std::cerr << clientCommand << "\nArguments:  " << arguments << "\n\n";
    }

    /*if(frameListener->getThreadStopRequested())
    {
        //TODO - log
        break;
    }*/
    return true;
}

void sendToAllClients(ODFrameListener *frameListener, std::string str)
{
    for (unsigned int i = 0; i < frameListener->mClientSockets.size(); ++i)
    {
        if (!frameListener->mClientSockets[i])
            continue;

        frameListener->mClientSockets[i]->send(str);
    }
}
