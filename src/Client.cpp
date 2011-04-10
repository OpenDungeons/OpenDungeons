#include <string>

#include "Globals.h"
#include "Socket.h"
#include "ODFrameListener.h"
#include "Network.h"
#include "ChatMessage.h"
#include "GameMap.h"
#include "Seat.h"
#include "Player.h"
#include "MapLight.h"
#include "Creature.h"
#include "ClientNotification.h"
#include "ProtectedObject.h"
#include "Weapon.h"
#include "ODApplication.h"

/*! \brief A thread function which runs on the client to handle communications with the server.
 *
 * A single instance of this thread is spawned by the client when it connects
 * to a server.  The socket connection itself is established before this thread
 * executes and the CSPStruct is used to pass this spawned socket instance, as
 * well as a pointer to the instance of the ExampleFrameListener being used by
 * the game.
 */
// THREAD - This function is meant to be called by pthread_create.
void *clientSocketProcessor(void *p)
{
    bool tempBool;
    std::string tempString;
    std::string serverCommand, arguments;
    Socket *sock = ((CSPStruct*) p)->nSocket;
    ODFrameListener *frameListener = ((CSPStruct*) p)->nFrameListener;
    delete (CSPStruct*) p;
    p = NULL;

    // Send a hello request to start the conversation with the server
    sem_wait(&sock->semaphore);
    sock->send(formatCommand("hello", std::string("OpenDungeons V ") + ODApplication::VERSION));
    sem_post(&sock->semaphore);
    while (sock->is_valid())
    {
        std::string commandFromServer = "";
        bool packetComplete;

        // Loop until we get to a place that ends in a '>' symbol
        // indicating that we have 1 or more FULL messages so we
        // don't break in the middle of a message.
        packetComplete = false;
        while (!packetComplete)
        {
            int charsRead = sock->recv(tempString);
            // If the server closed the connection
            if (charsRead <= 0)
            {
                // Place a chat message in the queue to inform
                // the user about the disconnect
                frameListener->chatMessages.push_back(new ChatMessage(
                        "SERVER_INFORMATION: ", "Server disconnect.",
                        time(NULL)));

                return NULL;
            }

            // Check to see if one or more complete packets in the buffer
            //FIXME:  This needs to be updated to include escaped closing brackets
            commandFromServer += tempString;
            if (commandFromServer[commandFromServer.length() - 1] == '>')
            {
                packetComplete = true;
            }
        }

        // Parse a command out of the bytestream from the server.
        //NOTE: This command is duplicated at the end of this do-while loop.
        bool parseReturnValue = parseCommand(commandFromServer, serverCommand,
                arguments);
        do
        {
            // This if-else chain functions like a switch statement
            // on the command recieved from the server.

            if (serverCommand.compare("picknick") == 0)
            {
                sem_wait(&sock->semaphore);
                sock->send(formatCommand("setnick", gameMap.me->nick));
                sem_post(&sock->semaphore);
            }

            /*
             else if(serverCommand.compare("yourseat") == 0)
             {
             std::stringstream tempSS(arguments);
             Seat *tempSeat = new Seat;
             cout << "\nAbout to read in seat.\n";
             cout.flush();
             tempSS >> tempSeat;
             gameMap.me->seat = tempSeat;
             sem_wait(&sock->semaphore);
             }
             */

            else if (serverCommand.compare("addseat") == 0)
            {
                std::stringstream tempSS(arguments);
                Seat *tempSeat = new Seat;
                tempSS >> tempSeat;
                gameMap.addEmptySeat(tempSeat);
                if (gameMap.me->seat == NULL)
                {
                    gameMap.me->seat = gameMap.popEmptySeat();
                }

                sem_wait(&sock->semaphore);
                sock->send(formatCommand("ok", "addseat"));
                sem_post(&sock->semaphore);
            }

            else if (serverCommand.compare("addplayer") == 0)
            {
                Player *tempPlayer = new Player;
                tempPlayer->nick = arguments;
                gameMap.addPlayer(tempPlayer);

                sem_wait(&sock->semaphore);
                sock->send(formatCommand("ok", "addplayer"));
                sem_post(&sock->semaphore);
            }

            else if (serverCommand.compare("chat") == 0)
            {
                ChatMessage *newMessage = processChatMessage(arguments);
                frameListener->chatMessages.push_back(newMessage);
            }

            else if (serverCommand.compare("newmap") == 0)
            {
                gameMap.clearAll();
            }

            else if (serverCommand.compare("turnsPerSecond") == 0)
            {
                ODApplication::turnsPerSecond = atof(arguments.c_str());
            }

            else if (serverCommand.compare("addtile") == 0)
            {
                std::stringstream tempSS(arguments);
                Tile *newTile = new Tile;
                tempSS >> newTile;
                gameMap.addTile(newTile);
                newTile->createMesh();
                sem_wait(&sock->semaphore);
                sock->send(formatCommand("ok", "addtile"));
                sem_post(&sock->semaphore);

                // Loop over the tile's neighbors to force them to recheck
                // their mesh to see if they can use an optimized one
                std::vector<Tile*> neighbors = gameMap.neighborTiles(newTile);
                for (unsigned int i = 0; i < neighbors.size(); ++i)
                {
                    neighbors[i]->setFullness(neighbors[i]->getFullness());
                }
            }

            else if (serverCommand.compare("addmaplight") == 0)
            {
                std::stringstream tempSS(arguments);
                MapLight *newMapLight = new MapLight;
                tempSS >> newMapLight;
                gameMap.addMapLight(newMapLight);
                newMapLight->createOgreEntity();
            }

            else if (serverCommand.compare("removeMapLight") == 0)
            {
                MapLight *tempMapLight = gameMap.getMapLight(arguments);
                tempMapLight->destroyOgreEntity();
                gameMap.removeMapLight(tempMapLight);
            }

            else if (serverCommand.compare("addroom") == 0)
            {
                std::stringstream tempSS(arguments);
                Room *newRoom = Room::createRoomFromStream(tempSS);
                gameMap.addRoom(newRoom);
                newRoom->createMeshes();
                sem_wait(&sock->semaphore);
                sock->send(formatCommand("ok", "addroom"));
                sem_post(&sock->semaphore);
            }

            else if (serverCommand.compare("addclass") == 0)
            {
                std::stringstream tempSS(arguments);
                ;
                CreatureClass *tempClass = new CreatureClass;

                tempSS >> tempClass;

                gameMap.addClassDescription(tempClass);
                sem_wait(&sock->semaphore);
                sock->send(formatCommand("ok", "addclass"));
                sem_post(&sock->semaphore);
            }

            else if (serverCommand.compare("addcreature") == 0)
            {
                //NOTE: This code is duplicated in readGameMapFromFile defined in src/Functions.cpp
                // Changes to this code should be reflected in that code as well
                Creature *newCreature = new Creature;

                std::stringstream tempSS;
                tempSS.str(arguments);
                tempSS >> newCreature;

                gameMap.addCreature(newCreature);
                newCreature->createMesh();
                newCreature->weaponL->createMesh();
                newCreature->weaponR->createMesh();
                sem_wait(&sock->semaphore);
                sock->send(formatCommand("ok", "addcreature"));
                sem_post(&sock->semaphore);
            }

            else if (serverCommand.compare("newturn") == 0)
            {
                std::stringstream tempSS;
                tempSS.str(arguments);
                long int tempLongInt;
                tempSS >> tempLongInt;
                turnNumber.set(tempLongInt);
            }

            else if (serverCommand.compare("animatedObjectAddDestination") == 0)
            {
                char array[255];

                std::stringstream tempSS;
                tempSS.str(arguments);

                tempSS.getline(array, sizeof(array), ':');
                //tempString = array;
                AnimatedObject *tempAnimatedObject = gameMap.getAnimatedObject(
                        array);

                double tempX, tempY, tempZ;
                tempSS.getline(array, sizeof(array), ':');
                tempX = atof(array);
                tempSS.getline(array, sizeof(array), ':');
                tempY = atof(array);
                tempSS.getline(array, sizeof(array));
                tempZ = atof(array);

                Ogre::Vector3 tempVector(tempX, tempY, tempZ);

                if (tempAnimatedObject != NULL)
                    tempAnimatedObject->addDestination(tempVector.x,
                            tempVector.y);
            }

            else if (serverCommand.compare("animatedObjectClearDestinations")
                    == 0)
            {
                AnimatedObject *tempAnimatedObject = gameMap.getAnimatedObject(
                        arguments);

                if (tempAnimatedObject != NULL)
                    tempAnimatedObject->clearDestinations();
            }

            //NOTE:  This code is duplicated in serverSocketProcessor()
            else if (serverCommand.compare("creaturePickUp") == 0)
            {
                char array[255];

                std::stringstream tempSS;
                tempSS.str(arguments);

                tempSS.getline(array, sizeof(array), ':');
                std::string playerNick = array;
                tempSS.getline(array, sizeof(array));
                std::string creatureName = array;

                Player *tempPlayer = gameMap.getPlayer(playerNick);
                Creature *tempCreature = gameMap.getCreature(creatureName);

                if (tempPlayer != NULL && tempCreature != NULL)
                {
                    tempPlayer->pickUpCreature(tempCreature);
                }
            }

            //NOTE:  This code is duplicated in serverSocketProcessor()
            else if (serverCommand.compare("creatureDrop") == 0)
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

                Player *tempPlayer = gameMap.getPlayer(playerNick);
                Tile *tempTile = gameMap.getTile(tempX, tempY);

                if (tempPlayer != NULL && tempTile != NULL)
                {
                    tempPlayer->dropCreature(tempTile);
                }
            }

            else if (serverCommand.compare("setObjectAnimationState") == 0)
            {

                char array[255];
                std::string tempState;
                std::stringstream tempSS;
                tempSS.str(arguments);

                // Parse the creature name and get a pointer to it
                tempSS.getline(array, sizeof(array), ':');
                Creature *tempCreature = gameMap.getCreature(array);

                // Parse the animation state
                tempSS.getline(array, sizeof(array), ':');
                tempState = array;

                tempSS.getline(array, sizeof(array));
                tempString = array;
                tempBool = (tempString.compare("true") == 0);

                if (tempCreature != NULL)
                {
                    tempCreature->setAnimationState(tempState, tempBool);
                }

            }

            else if (serverCommand.compare("tileFullnessChange") == 0)
            {
                char array[255];
                double tempFullness, tempX, tempY;
                std::stringstream tempSS;

                tempSS.str(arguments);
                tempSS.getline(array, sizeof(array), ':');
                tempFullness = atof(array);
                tempSS.getline(array, sizeof(array), ':');
                tempX = atof(array);
                tempSS.getline(array, sizeof(array));
                tempY = atof(array);

                Tile *tempTile = gameMap.getTile(tempX, tempY);
                if (tempTile != NULL)
                {
                    tempTile->setFullness(tempFullness);
                }
                else
                {
                    cerr
                            << "\nERROR:  Server told us to set the fullness for a nonexistent tile.\n";
                }
            }

            else
            {
                cerr << "\n\n\nERROR:  Unknown server command!\nCommand:";
                cerr << serverCommand << "\nArguments:" << arguments << "\n\n";
                exit(1);
            }

            //NOTE: This command is duplicated at the beginning of this do-while loop.
            parseReturnValue = parseCommand(commandFromServer, serverCommand,
                    arguments);
        } while (parseReturnValue);
    }

    // Return something to make the compiler happy
    return NULL;
}

/*! \brief The thread which monitors the clientNotificationQueue for new events and informs the server about them.
 *
 * This thread runs on the client and acts as a "consumer" on the
 * clientNotificationQueue.  It takes an event out of the queue, determines
 * which clients need to be informed about that particular event, and
 * dispacthes TCP packets to inform the clients about the new information.
 */
// THREAD - This function is meant to be called by pthread_create.
void *clientNotificationProcessor(void *p)
{
    std::stringstream tempSS;
    Tile *tempTile;
    Creature *tempCreature;
    Player *tempPlayer;
    bool flag;

    while (true)
    {
        // Wait until a message is place in the queue
        sem_wait(&clientNotificationQueueSemaphore);

        // Take a message out of the front of the notification queue
        sem_wait(&clientNotificationQueueLockSemaphore);
        ClientNotification* event = clientNotificationQueue.front();
        clientNotificationQueue.pop_front();
        sem_post(&clientNotificationQueueLockSemaphore);

        switch (event->type)
        {
            case ClientNotification::creaturePickUp:
                tempCreature = (Creature*) event->p;
                tempPlayer = (Player*) event->p2;

                tempSS.str("");
                tempSS << tempPlayer->nick << ":" << tempCreature->name;

                sem_wait(&clientSocket->semaphore);
                clientSocket->send(
                        formatCommand("creaturePickUp", tempSS.str()));
                sem_post(&clientSocket->semaphore);
                break;

            case ClientNotification::creatureDrop:
                tempPlayer = (Player*) event->p;
                tempTile = (Tile*) event->p2;

                tempSS.str("");
                tempSS << tempPlayer->nick << ":" << tempTile->x << ":"
                        << tempTile->y;

                sem_wait(&clientSocket->semaphore);
                clientSocket->send(formatCommand("creatureDrop", tempSS.str()));
                sem_post(&clientSocket->semaphore);
                break;

            case ClientNotification::markTile:
                tempTile = (Tile*) event->p;
                flag = event->flag;
                tempSS.str("");
                tempSS << tempTile->x << ":" << tempTile->y << ":"
                        << (flag ? "true" : "false");

                sem_wait(&clientSocket->semaphore);
                clientSocket->send(formatCommand("markTile", tempSS.str()));
                sem_post(&clientSocket->semaphore);
                break;

            default:
                cerr << "\n\nERROR:  Unhandled ClientNotification type encoutered!\n\n";

                //TODO:  Remove me later - this is to force a core dump so I can debug why this happenened
                Creature * throwAsegfault = NULL;
                throwAsegfault->getPosition();

                exit(1);
                break;
        }
    }
}
