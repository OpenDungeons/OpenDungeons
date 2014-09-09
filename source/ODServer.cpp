/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ODServer.h"

#include "ODClient.h"
#include "ServerNotification.h"
#include "GameMap.h"
#include "ODApplication.h"
#include "MapLight.h"
#include "ChatMessage.h"
#include "ODConsoleCommand.h"
#include "LogManager.h"

#include <SFML/Network.hpp>
#include <SFML/System.hpp>

const std::string ODServer::SERVER_INFORMATION = "SERVER_INFORMATION";

template<> ODServer* Ogre::Singleton<ODServer>::msSingleton = 0;

ODServer::ODServer() :
    mNbClientsNotReady(0),
    mGameMap(new GameMap(true))
{
}

ODServer::~ODServer()
{
    delete mGameMap;
}

bool ODServer::startServer(const std::string& levelFilename, bool replaceHumanPlayersByAi)
{
    LogManager& logManager = LogManager::getSingleton();

    mLevelFilename = levelFilename;

    // Start the server socket listener as well as the server socket thread
    if (isConnected())
    {
        logManager.logMessage("Couldn't start server: The server is already connected");
        return false;
    }
    if (ODClient::getSingleton().isConnected())
    {
        logManager.logMessage("Couldn't start server: The client is already connected");
        return false;
    }

    // Read in the map. The map loading should be happen here and not in the server thread to
    // make sure it is valid before launching the server.
    GameMap* gameMap = mGameMap;
    if (!gameMap->loadLevel(levelFilename))
    {
        logManager.logMessage("Couldn't start server. The level file can't be loaded: " + levelFilename);
        return false;
    }

    // Fill seats with either player, AIs or nothing depending on the given faction.
    uint32_t i = 0;
    uint32_t uniqueAINumber = 0;
    uint32_t nbPlayerSeat = 0;
    mNbClientsNotReady = 0;
    while (i < gameMap->numEmptySeats())
    {
        Seat* seat = gameMap->getEmptySeat(i);

        if (seat->mFaction == "Player")
        {
            ++nbPlayerSeat;
            // Add local player on first slot available.
            if(replaceHumanPlayersByAi && nbPlayerSeat > 1)
            {
                // NOTE - AI should later have definable names maybe?.
                Player* aiPlayer = new Player();
                aiPlayer->setNick("Keeper AI " + Ogre::StringConverter::toString(uniqueAINumber++));

                // The empty seat is removed by addPlayer(), so we loop without incrementing i
                gameMap->addPlayer(aiPlayer, gameMap->popEmptySeat(seat->getColor()));
                gameMap->assignAI(*aiPlayer, "KeeperAI");
                continue;
            }
            else
            {
                ++mNbClientsNotReady;
            }
        }
        else if (seat->mFaction == "KeeperAI")
        {
            // NOTE - AI should later have definable names maybe?.
            Player* aiPlayer = new Player();
            aiPlayer->setNick("Keeper AI " + Ogre::StringConverter::toString(++uniqueAINumber));

            // The empty seat is removed by addPlayer(), so we loop without incrementing i
            gameMap->addPlayer(aiPlayer, gameMap->popEmptySeat(seat->getColor()));
            gameMap->assignAI(*aiPlayer, "KeeperAI");
            continue;
        }

        ++i;
    }

    logManager.logMessage("Added: " + Ogre::StringConverter::toString(nbPlayerSeat) + " Human players");
    logManager.logMessage("Added: " + Ogre::StringConverter::toString(uniqueAINumber) + " AI players");

    // If no player seat, the game cannot be launched
    if (nbPlayerSeat == 0)
        return false;

    // Set up the socket to listen on the specified port
    if (!createServer(ODApplication::PORT_NUMBER))
    {
        logManager.logMessage("ERROR:  Server could not create server socket!");
        return false;
    }

    return true;
}

void ODServer::queueServerNotification(ServerNotification* n)
{
    if ((n == NULL) || (!isConnected()))
        return;
    mServerNotificationQueue.push_back(n);
}

void ODServer::queueConsoleCommand(ODConsoleCommand* cc)
{
    if ((cc == NULL) || (!isConnected()))
        return;
    mConsoleCommandQueue.push_back(cc);
}

void ODServer::processServerCommandQueue()
{
    bool running = true;
    GameMap* gameMap = mGameMap;

    while(running)
    {
        // If the queue is empty, let's get out of the loop.
        if (mConsoleCommandQueue.empty())
            break;

        // Take a message out of the front of the notification queue
        ODConsoleCommand *command = mConsoleCommandQueue.front();
        mConsoleCommandQueue.pop_front();

        if (command == NULL)
            continue;

        command->execute(gameMap);

        delete command;
    }
}

void ODServer::startNewTurn(double timeSinceLastFrame)
{
    GameMap* gameMap = mGameMap;
    int64_t turn = gameMap->getTurnNumber();

    // We wait until every client acknowledge the turn to start the next one. This way, we ensure
    // synchronisation is not too bad
    for (std::vector<ODSocketClient*>::iterator it = mSockClients.begin(); it != mSockClients.end(); ++it)
    {
        ODSocketClient* client = *it;
        if(client->getLastTurnAck() != turn)
            return;
    }

    gameMap->setTurnNumber(++turn);
    try
    {
        ServerNotification* serverNotification = new ServerNotification(
            ServerNotification::turnStarted, NULL);
        serverNotification->mPacket << turn;
        queueServerNotification(serverNotification);
    }
    catch (std::bad_alloc&)
    {
        LogManager::getSingletonPtr()->logMessage("ERROR:  bad alloc in turnStarted", Ogre::LML_CRITICAL);
        exit(1);
    }

    gameMap->updateAnimations(timeSinceLastFrame);

    // We notify the clients about what they got
    for (std::vector<ODSocketClient*>::iterator it = mSockClients.begin(); it != mSockClients.end(); ++it)
    {
        ODSocketClient* sock = *it;
        Player* player = sock->getPlayer();
        try
        {
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotification::refreshPlayerSeat, player);
            std::string goals = gameMap->getGoalsStringForPlayer(player);
            Seat* seat = player->getSeat();
            serverNotification->mPacket << seat << goals;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
        catch (std::bad_alloc&)
        {
            OD_ASSERT_TRUE(false);
            exit(1);
        }

        // Here, the creature list is pulled. It could be possible that the creature dies before the stat window is
        // closed. So, if we cannot find the creature, we just erase it.
        std::vector<std::string>& creatures = mCreaturesInfoWanted[sock];
        std::vector<std::string>::iterator itCreatures = creatures.begin();
        while(itCreatures != creatures.end())
        {
            std::string& name = *itCreatures;
            Creature* creature = gameMap->getCreature(name);
            if(creature == NULL)
                itCreatures = creatures.erase(itCreatures);
            else
            {
                std::string creatureInfos = creature->getStatsText();
                try
                {
                    ServerNotification *serverNotification = new ServerNotification(
                        ServerNotification::notifyCreatureInfo, player);
                    serverNotification->mPacket << name << creatureInfos;
                    ODServer::getSingleton().queueServerNotification(serverNotification);
                }
                catch (std::bad_alloc&)
                {
                    OD_ASSERT_TRUE(false);
                    exit(1);
                }
                ++itCreatures;
            }
        }
    }

    gameMap->doTurn();
    gameMap->processDeletionQueues();
    gameMap->doPlayerAITurn(timeSinceLastFrame);
}

void ODServer::serverThread()
{
    GameMap* gameMap = mGameMap;
    sf::Clock clock;
    double turnLengthMs = 1000.0 / ODApplication::turnsPerSecond;
    while(isConnected())
    {
        // doTask sould return after the length of 1 turn even if their are communications. When
        // it returns, we can launch next turn.
        doTask(static_cast<int32_t>(turnLengthMs));

        if(gameMap->getTurnNumber() == -1)
        {
            // The game is not started
            if(mNbClientsNotReady == 0)
            {
                // Every client is connected and ready, we can launch the game
                try
                {
                    // Send turn 0 to init the map
                    ServerNotification* serverNotification = new ServerNotification(
                        ServerNotification::turnStarted, NULL);
                    serverNotification->mPacket << static_cast<int64_t>(0);
                    queueServerNotification(serverNotification);
                }
                catch (std::bad_alloc&)
                {
                    LogManager::getSingleton().logMessage("ERROR: bad alloc in turnStarted", Ogre::LML_CRITICAL);
                    exit(1);
                }
                LogManager::getSingleton().logMessage("Server ready, starting game");
                gameMap->setTurnNumber(0);
                gameMap->setGamePaused(false);
                gameMap->createAllEntities();

                // Fill starting gold
                for(std::vector<Seat*>::iterator it = gameMap->filledSeats.begin(); it != gameMap->filledSeats.end(); ++it)
                {
                    Seat* seat = *it;
                    if(seat->mStartingGold > 0)
                        gameMap->addGoldToSeat(seat->mStartingGold, seat->mColor);
                }
            }
            else
            {
                // We are still waiting for players
                continue;
            }
        }

        // After starting a new turn, we should process server notifications
        // before processing client messages. Otherwise, we could have weird issues
        // like allow picking up a dead creature for example.
        // We make sure the server time is a little bit late regarding the clients to
        // make sure server is not more advanced than clients. We do that because it is better for clients
        // to wait for server. If server is in advance, he might send commands before the
        // creatures arrive at their destination. That could result in weird issues like
        // creatures goinf through walls.
        startNewTurn(static_cast<double>(clock.restart().asSeconds()) * 0.95);

        processServerNotifications();

        processServerCommandQueue();
    }
}

void ODServer::processServerNotifications()
{
    GameMap* gameMap = mGameMap;

    bool running = true;

    while (running)
    {
        // If the queue is empty, let's get out of the loop.
        if (mServerNotificationQueue.empty())
            break;

        // Take a message out of the front of the notification queue
        ServerNotification *event = mServerNotificationQueue.front();
        mServerNotificationQueue.pop_front();

        //FIXME:  This really should never happen but the queue does occasionally pop a NULL.
        //This is probably a bug somewhere else where a NULL is being place in the queue.
        if (event == NULL)
            continue;

        switch (event->mType)
        {
            case ServerNotification::turnStarted:
                LogManager::getSingleton().logMessage("Server sends newturn="
                    + Ogre::StringConverter::toString((int32_t)gameMap->getTurnNumber()));
                sendToAllClients(event->mPacket);
                break;

            case ServerNotification::setTurnsPerSecond:
                // This one is not used on client side. Shall we remove it?
                sendToAllClients(event->mPacket);
                break;

            case ServerNotification::refreshPlayerSeat:
            {
                // For now, only the player whose seat changed is notified. If we need it,
                // we could send the event to every player so that they can see how far from
                // the goals the other players are
                ODSocketClient* client = getClientFromPlayer(event->mConcernedPlayer);
                OD_ASSERT_TRUE_MSG(client != NULL, "name=" + event->mConcernedPlayer->getNick());
                if(client != NULL)
                    sendMsgToClient(client, event->mPacket);
                break;
            }

            case ServerNotification::pickupCreature:
                // This should not be a message as it is sent directly to the client
                OD_ASSERT_TRUE(false);
                break;

            case ServerNotification::dropCreature:
                // This should not be a message as it is sent directly to the client
                OD_ASSERT_TRUE(false);
                break;

            case ServerNotification::creaturePickedUp:
                // This should not be a message as it is sent directly to the client
                OD_ASSERT_TRUE(false);
                break;

            case ServerNotification::creatureDropped:
                // This should not be a message as it is sent directly to the client
                OD_ASSERT_TRUE(false);
                break;

            case ServerNotification::buildRoom:
            {
                // This message should only be sent by ai players (human players are notified directly)
                std::string& faction = event->mConcernedPlayer->getSeat()->mFaction;
                OD_ASSERT_TRUE_MSG(faction != "Player", faction);
                sendToAllClients(event->mPacket);
                break;
            }

            case ServerNotification::buildTrap:
                // This should not be a message as it is sent directly to the client
                // TODO : this should be used like buildRoom when the AI will build traps
                OD_ASSERT_TRUE(false);
                break;

            case ServerNotification::markTiles:
                // This should not be a message as it is sent directly to the client
                OD_ASSERT_TRUE(false);
                break;

            case ServerNotification::exit:
                running = false;
                stopServer();
                break;

            default:
                sendToAllClients(event->mPacket);
                break;
        }

        delete event;
        event = NULL;
    }
}

bool ODServer::processClientNotifications(ODSocketClient* clientSocket)
{
    if (!clientSocket)
        return false;

    GameMap* gameMap = mGameMap;

    ODPacket packetReceived;

    ODSocketClient::ODComStatus status = receiveMsgFromClient(clientSocket, packetReceived);

    // If the client closed the connection
    if (status != ODSocketClient::ODComStatus::OK)
    {
        return (status != ODSocketClient::ODComStatus::Error);
    }

    ClientNotification::ClientNotificationType clientCommand;
    OD_ASSERT_TRUE(packetReceived >> clientCommand);

    switch(clientCommand)
    {
        case ClientNotification::hello:
        {
            if(std::string("connected").compare(clientSocket->getState()) != 0)
                return false;
            std::string version;
            OD_ASSERT_TRUE(packetReceived >> version);

            // If the version is different, we refuse the client
            if(version.compare(std::string("OpenDungeons V ") + ODApplication::VERSION) != 0)
            {
                LogManager::getSingleton().logMessage("Server rejected client. Application version mismatch: required= "
                    + ODApplication::VERSION + ", received=" + version);
                return false;
            }

            // Tell the client to load the given map
            setClientState(clientSocket, "loadLevel");
            ODPacket packetSend;
            packetSend << ServerNotification::loadLevel << mLevelFilename;
            sendMsgToClient(clientSocket, packetSend);
            break;
        }

        case ClientNotification::levelOK:
        {
            if(std::string("loadLevel").compare(clientSocket->getState()) != 0)
                return false;

            setClientState(clientSocket, "nick");
            // Tell the client to give us their nickname
            ODPacket packetSend;
            packetSend << ServerNotification::pickNick;
            sendMsgToClient(clientSocket, packetSend);
            break;
        }

        case ClientNotification::setNick:
        {
            if(std::string("nick").compare(clientSocket->getState()) != 0)
                return false;

            ODPacket packetSend;
            // Pick nick
            std::string clientNick;
            OD_ASSERT_TRUE(packetReceived >> clientNick);

            // Create a player structure for the client
            Player *curPlayer;
            Seat* seat = gameMap->getEmptySeat("Player");
            curPlayer = new Player;
            curPlayer->setNick(clientNick);
            // The seat should be available since it has been checked before accepting the client connexion
            if(seat != NULL)
            {
                int color = seat->getColor();
                clientSocket->setPlayer(curPlayer);
                gameMap->addPlayer(curPlayer, gameMap->popEmptySeat(color));
                // We notify the newly connected player to the others
                packetSend.clear();
                packetSend << ServerNotification::addPlayer << clientNick
                    << color;
                for (std::vector<ODSocketClient*>::iterator it = mSockClients.begin(); it != mSockClients.end(); ++it)
                {
                    ODSocketClient *client = *it;
                    if(clientSocket != client)
                    {
                        sendMsgToClient(client, packetSend);
                    }
                }
            }
            else
            {
                // No seat available. We disconnect the client
                delete curPlayer;
                return false;
            }

            // Send over the information about the game. Clients can connect only before
            // a game is launched (it would be too long to transfer all data otherwise).
            // So we assume that they have loaded the same map (we have tested it as well
            // as we could previously). We just need to send them information about the
            // players
            packetSend.clear();
            packetSend << ServerNotification::turnsPerSecond << ODApplication::turnsPerSecond;
            sendMsgToClient(clientSocket, packetSend);

            packetSend.clear();
            int color = seat->getColor();
            packetSend << ServerNotification::yourSeat << color;
            sendMsgToClient(clientSocket, packetSend);

            // We notify the pending players to the new one
            for (unsigned int i = 0; i < gameMap->numPlayers(); ++i)
            {
                // Don't tell the client about its own player structure
                Player* tempPlayer = gameMap->getPlayer(i);
                if (curPlayer != tempPlayer && tempPlayer != NULL)
                {
                    packetSend.clear();
                    int color = tempPlayer->getSeat()->getColor();
                    std::string nick = tempPlayer->getNick();
                    packetSend << ServerNotification::addPlayer << nick << color;
                    sendMsgToClient(clientSocket, packetSend);
                }
            }

             // The player received everything. He is ready
             setClientState(clientSocket, "ready");
            --mNbClientsNotReady;
            LogManager::getSingleton().logMessage("Player=" + curPlayer->getNick()
                + " has been accepted in the game on color="
                + Ogre::StringConverter::toString(seat->getColor()));
            break;
        }

        case ClientNotification::chat:
        {
            // TODO : handle chat for everybody/allies/player
            // As chat message do not interfere with GameMap, it is OK to send
            // them directly to the clients instead of queuing a ServerNotification
            // to the Server
            std::string chatMsg;
            OD_ASSERT_TRUE(packetReceived >> chatMsg);
            Player* player = clientSocket->getPlayer();
            ODPacket packetSend;
            std::string nick = player->getNick();
            packetSend << ServerNotification::chat << nick << chatMsg;
            sendToAllClients(packetSend);
            break;
        }

        case ClientNotification::askCreaturePickUp:
        {
            std::string creatureName;
            OD_ASSERT_TRUE(packetReceived >> creatureName);

            Player *player = clientSocket->getPlayer();
            Creature *creature = gameMap->getCreature(creatureName);
            OD_ASSERT_TRUE_MSG(creature != NULL, "name=" + creatureName);
            if (creature != NULL)
            {
                int color = creature->getColor();
                if(color == player->getSeat()->getColor())
                {
                    player->pickUpCreature(creature);
                    // We notify the player that he pickedup the creature
                    ODPacket packet;
                    packet << ServerNotification::pickupCreature;
                    packet << creatureName;
                    sendMsgToClient(clientSocket, packet);

                    // We notify the other players
                    for (std::vector<ODSocketClient*>::iterator it = mSockClients.begin(); it != mSockClients.end(); ++it)
                    {
                        ODSocketClient* tmpClient = *it;
                        if(tmpClient->getPlayer() != player)
                        {
                            // We notify the other players that a creature has been picked up
                            ODPacket packetSend;
                            packetSend << ServerNotification::creaturePickedUp;
                            packetSend << color << creatureName;
                            sendMsgToClient(tmpClient, packetSend);
                        }
                    }
                }
                else
                {
                    LogManager::getSingleton().logMessage("player=" + player->getNick()
                        + " tried to pick up creature from different color=" + creatureName);
                }
            }
            break;
        }

        case ClientNotification::askCreatureDrop:
        {
            Tile tmpTile(gameMap);
            OD_ASSERT_TRUE(packetReceived >> &tmpTile);
            Player *player = clientSocket->getPlayer();
            Tile* tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            OD_ASSERT_TRUE_MSG(tile != NULL, "tile=" + Ogre::StringConverter::toString(tmpTile.getX())
                + "," + Ogre::StringConverter::toString(tmpTile.getY()));
            if(tile != NULL)
            {
                if(player->isDropCreaturePossible(tile))
                {
                    player->dropCreature(tile);
                    int color = player->getSeat()->getColor();
                    ODPacket packet;
                    packet << ServerNotification::dropCreature;
                    packet << tile;
                    sendMsgToClient(clientSocket, packet);

                    // We also notify the other players
                    for (std::vector<ODSocketClient*>::iterator it = mSockClients.begin(); it != mSockClients.end(); ++it)
                    {
                        ODSocketClient* tmpClient = *it;
                        if(tmpClient->getPlayer() != player)
                        {
                            packet.clear();
                            packet << ServerNotification::creatureDropped;
                            packet << color << tile;
                            sendMsgToClient(tmpClient, packet);
                        }
                    }
                }
                else
                {
                    LogManager::getSingleton().logMessage("player=" + player->getNick()
                        + " could not drop creature in hand on tile "
                        + Ogre::StringConverter::toString(tile->getX())
                        + "," + Ogre::StringConverter::toString(tile->getY()));
                }
            }
            break;
        }

        case ClientNotification::askMarkTile:
        {
            int x1, y1, x2, y2;
            bool isDigSet;
            Player* player = clientSocket->getPlayer();

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2 >> isDigSet);
            std::vector<Tile*> tiles = gameMap->getDiggableTilesForPlayerInArea(x1,
                y1, x2, y2, player);
            if(!tiles.empty())
            {
                // We send to the client the list of tiles to mark. We send to him only
                ODPacket packet;
                packet << ServerNotification::markTiles;
                int nbTiles = tiles.size();
                packet << isDigSet << nbTiles;
                for(std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end(); ++it)
                {
                    Tile* tile = *it;
                    packet << tile;
                    // We also update the server game map
                    tile->setMarkedForDigging(isDigSet, player);
                }
                sendMsgToClient(clientSocket, packet);
            }
            break;
        }

        case ClientNotification::askBuildRoom:
        {
            int x1, y1, x2, y2;
            int intType;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2 >> intType);
            Player* player = clientSocket->getPlayer();
            std::vector<Tile*> tiles = gameMap->getBuildableTilesForPlayerInArea(x1,
                y1, x2, y2, player);

            if(!tiles.empty())
            {
                Room::RoomType type = static_cast<Room::RoomType>(intType);
                int goldRequired = tiles.size() * Room::costPerTile(type);
                if(gameMap->withdrawFromTreasuries(goldRequired, player->getSeat()))
                {
                    Room* room = gameMap->buildRoomForPlayer(tiles, type, player);
                    // We build the message for the new room creation here with the original room size because
                    // it may change if a room is absorbed
                    ODPacket packet;
                    packet << ServerNotification::buildRoom;
                    int nbTiles = tiles.size();
                    int color = player->getSeat()->getColor();
                    const std::string& name = room->getName();
                    packet << name << intType << color << nbTiles;
                    for(std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end(); ++it)
                    {
                        Tile* tile = *it;
                        packet << tile;
                    }
                    sendToAllClients(packet);
                }
            }
            break;
        }

        case ClientNotification::askBuildTrap:
        {
            int x1, y1, x2, y2;
            int intType;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2 >> intType);
            Player* player = clientSocket->getPlayer();
            std::vector<Tile*> tiles = gameMap->getBuildableTilesForPlayerInArea(x1,
                y1, x2, y2, player);

            if(!tiles.empty())
            {
                Trap::TrapType type = static_cast<Trap::TrapType>(intType);
                int goldRequired = tiles.size() * Trap::costPerTile(type);
                if(gameMap->withdrawFromTreasuries(goldRequired, player->getSeat()))
                {
                    Trap* trap = gameMap->buildTrapForPlayer(tiles, type, player);
                    const std::string& name = trap->getName();
                    ODPacket packet;
                    packet << ServerNotification::buildTrap;
                    int nbTiles = tiles.size();
                    int color = player->getSeat()->getColor();
                    packet << name << intType << color << nbTiles;
                    for(std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end(); ++it)
                    {
                        Tile* tile = *it;
                        packet << tile;
                    }
                    sendToAllClients(packet);
                }
            }
            break;
        }

        case ClientNotification::ackNewTurn:
        {
            int64_t turn;
            OD_ASSERT_TRUE(packetReceived >> turn);
            clientSocket->setLastTurnAck(turn);
            break;
        }

        case ClientNotification::askCreatureInfos:
        {
            std::string name;
            bool refreshEachTurn;
            OD_ASSERT_TRUE(packetReceived >> name >> refreshEachTurn);
            std::vector<std::string>& creatures = mCreaturesInfoWanted[clientSocket];

            std::vector<std::string>::iterator it = std::find(creatures.begin(), creatures.end(), name);
            if(refreshEachTurn && (it == creatures.end()))
            {
                creatures.push_back(name);
            }
            else if(!refreshEachTurn && (it != creatures.end()))
                creatures.erase(it);

            break;
        }

        default:
        {
            LogManager::getSingleton().logMessage("ERROR:  Unhandled command received from client:"
                + Ogre::StringConverter::toString(clientCommand));
        }
    }

    return true;
}

void ODServer::sendToAllClients(ODPacket& packetSend)
{
    // TODO : except the console, nothing should decide to whom a cmd should be sent. Check if it is true
    sendMsgToAllClients(packetSend);
}

bool ODServer::notifyNewConnection(ODSocketClient *clientSocket)
{
    GameMap* gameMap = mGameMap;
    if (gameMap == NULL)
        return false;

    setClientState(clientSocket, "connected");
    // TODO : the seat should be popped and saved in the client so that we can keep track if several clients
    // connects at same time (otherwise, clients could be accepted even if no seat available). But this
    // would require to be careful because if the connection is refused for
    // some reason, it has to be turned available again for another player
    Seat* seat = gameMap->getEmptySeat("Player");
    return (seat != NULL);
}

bool ODServer::notifyClientMessage(ODSocketClient *clientSocket)
{
    bool ret = processClientNotifications(clientSocket);
    if(!ret)
    {
        try
        {
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotification::chatServer, clientSocket->getPlayer());
            std::string msg = clientSocket->getPlayer()->getNick()
                + " disconnected";
            serverNotification->mPacket << msg;
            queueServerNotification(serverNotification);
        }
        catch (std::bad_alloc&)
        {
            Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in ODServer::processClientNotifications", Ogre::LML_CRITICAL);
            exit(1);
        }
        // TODO : wait at least 1 minute if the client reconnects
    }
    return ret;
}

void ODServer::stopServer()
{
    // We start by stopping server to make sure no new message comes
    ODSocketServer::stopServer();

    mNbClientsNotReady = 0;

    // Now that the server is stopped, we can remove all pending messages
    while(!mServerNotificationQueue.empty())
    {
        delete mServerNotificationQueue.front();
        mServerNotificationQueue.pop_front();
    }
    mGameMap->clearAll();
    mGameMap->processDeletionQueues();
}

void ODServer::notifyExit()
{
    ODSocketServer::stopServer();
    while(!mServerNotificationQueue.empty())
    {
        delete mServerNotificationQueue.front();
        mServerNotificationQueue.pop_front();
    }

    try
    {
        ServerNotification* exitServerNotification = new ServerNotification(
            ServerNotification::exit, NULL);
        queueServerNotification(exitServerNotification);
    }
    catch (std::bad_alloc&)
    {
        Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in MovableGameEntity::clearDestinations", Ogre::LML_CRITICAL);
        exit(1);
    }
}

ODSocketClient* ODServer::getClientFromPlayer(Player* player)
{
    ODSocketClient* ret = NULL;

    for (std::vector<ODSocketClient*>::iterator it = mSockClients.begin(); it != mSockClients.end(); ++it)
    {
        ODSocketClient* client = *it;
        if(client->getPlayer() == player)
        {
            ret = client;
            break;
        }
    }

    return ret;
}
