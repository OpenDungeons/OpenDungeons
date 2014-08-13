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
    if (!gameMap->LoadLevel(levelFilename))
        return false;

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
        serverNotification->packet << turn;
        queueServerNotification(serverNotification);
    }
    catch (std::bad_alloc&)
    {
        LogManager::getSingletonPtr()->logMessage("ERROR:  bad alloc in turnStarted", Ogre::LML_CRITICAL);
        exit(1);
    }

    gameMap->updateAnimations(timeSinceLastFrame);
    gameMap->doTurn();
    gameMap->processDeletionQueues();
    gameMap->doPlayerAITurn(timeSinceLastFrame);
}

void ODServer::serverThread()
{
    GameMap* gameMap = mGameMap;
    while(isConnected())
    {
        double turnLengthMs = 1000.0 / ODApplication::turnsPerSecond;
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
                    serverNotification->packet << static_cast<int64_t>(0);
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
            }
            else
            {
                // We are still waiting for players
                continue;
            }
        }

        startNewTurn(turnLengthMs / 1000.0);

        processServerNotifications();
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
                sendToAllClients(event->packet);
                break;

            case ServerNotification::setTurnsPerSecond:
                // This one is not used on client side. Shall we remove it?
                sendToAllClients(event->packet);
                break;

            case ServerNotification::creaturePickedUp:
            {
                // We notify the clients except the one who picked it up
                ServerNotification::ServerNotificationType type;
                int color;
                std::string creatureName;
                OD_ASSERT_TRUE(event->packet >> type >> color >> creatureName);
                OD_ASSERT_TRUE(type == event->mType);
                for (std::vector<ODSocketClient*>::iterator it = mSockClients.begin(); it != mSockClients.end(); ++it)
                {
                    ODSocketClient* tmpClient = *it;
                    if(tmpClient->getPlayer() == event->mConcernedPlayer)
                    {
                        // We notify the player that he dropped the creature
                        ODPacket packetSend;
                        packetSend << ServerNotification::pickupCreature;
                        packetSend << creatureName;
                        sendMsgToClient(tmpClient, packetSend);
                    }
                    else
                    {
                        // We notify the other players that a creature has been dropped
                        ODPacket packetSend;
                        packetSend << ServerNotification::creaturePickedUp;
                        packetSend << color << creatureName;
                        sendMsgToClient(tmpClient, packetSend);
                    }
                }
                break;
            }

            case ServerNotification::creatureDropped:
            {
                // We notify the clients except the one who picked it up
                ServerNotification::ServerNotificationType type;
                int color;
                Tile tmpTile(gameMap);
                OD_ASSERT_TRUE(event->packet >> type >> color >> &tmpTile);
                OD_ASSERT_TRUE(type == event->mType);
                for (std::vector<ODSocketClient*>::iterator it = mSockClients.begin(); it != mSockClients.end(); ++it)
                {
                    ODSocketClient* tmpClient = *it;
                    if(tmpClient->getPlayer() == event->mConcernedPlayer)
                    {
                        // We notify the player that he dropped the creature
                        ODPacket packetSend;
                        packetSend << ServerNotification::dropCreature;
                        packetSend << &tmpTile;
                        sendMsgToClient(tmpClient, packetSend);
                    }
                    else
                    {
                        // We notify the other players that a creature has been dropped
                        ODPacket packetSend;
                        packetSend << ServerNotification::creatureDropped;
                        packetSend << color << &tmpTile;
                        sendMsgToClient(tmpClient, packetSend);
                    }
                }
                break;
            }

            case ServerNotification::refreshPlayerSeat:
            {
                // For now, only the player whose seat changed is notified. If we need it,
                // we could send the event to every player so that they can see how far from
                // the goals the other players are
                ODSocketClient* client = getClientFromPlayer(event->mConcernedPlayer);
                if(client != NULL)
                    sendMsgToClient(client, event->packet);
                else
                    LogManager::getSingleton().logMessage(
                        std::string("ERROR : getClientFromPlayer returned NULL for player ")
                        + event->mConcernedPlayer->getNick());
                break;
            }

            case ServerNotification::markTiles:
            {
                ODSocketClient* client = getClientFromPlayer(event->mConcernedPlayer);
                if(client != NULL)
                    sendMsgToClient(client, event->packet);
                else
                    LogManager::getSingleton().logMessage(
                        std::string("ERROR : getClientFromPlayer returned NULL for player ")
                        + event->mConcernedPlayer->getNick());
                break;
            }

            case ServerNotification::exit:
                running = false;
                stopServer();
                break;

            default:
                sendToAllClients(event->packet);
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
            ODPacket packetSend;
            std::string version;
            std::string levelFilename;
            OD_ASSERT_TRUE(packetReceived >> version >> levelFilename);

            // If the levelFilename is different, we refuse the client
            if(levelFilename.compare(mLevelFilename) != 0)
            {
                LogManager::getSingleton().logMessage("Server rejected client awaited map= "
                    + mLevelFilename + ", received=" + levelFilename);
                return false;
            }

            setClientState(clientSocket, "nick");
            // Tell the client to give us their nickname and to clear their map
            packetSend.clear();
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

            if (creature != NULL)
            {
                if(creature->getColor() == player->getSeat()->getColor())
                {
                    player->pickUpCreature(creature);
                    try
                    {
                        int color = player->getSeat()->getColor();
                        ServerNotification *serverNotification = new ServerNotification(
                            ServerNotification::creaturePickedUp, player);
                        serverNotification->packet << color << creatureName;
                        queueServerNotification(serverNotification);
                    }
                    catch (std::bad_alloc&)
                    {
                        Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in GameMap::doTurn", Ogre::LML_CRITICAL);
                        exit(1);
                    }
                }
                else
                {
                    LogManager::getSingleton().logMessage("ERROR : player=" + player->getNick()
                        + " tried to pick up creature from different color=" + creatureName);
                }
            }
            else
            {
                LogManager::getSingleton().logMessage("ERROR : player " + player->getNick()
                    + " could not pick up creature " + creatureName);
            }
            break;
        }

        case ClientNotification::askCreatureDrop:
        {
            Tile tmpTile(gameMap);
            OD_ASSERT_TRUE(packetReceived >> &tmpTile);
            Player *player = clientSocket->getPlayer();
            Tile* tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            if(tile != NULL)
            {
                if(player->isDropCreaturePossible(tile))
                {
                    player->dropCreature(tile);
                    int color = player->getSeat()->getColor();
                    try
                    {
                        ServerNotification *serverNotification = new ServerNotification(
                            ServerNotification::creatureDropped, player);
                        serverNotification->packet << color << tile;
                        queueServerNotification(serverNotification);
                    }
                    catch (std::bad_alloc&)
                    {
                        Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in GameMap::doTurn", Ogre::LML_CRITICAL);
                        exit(1);
                    }
                }
                else
                {
                    LogManager::getSingleton().logMessage("ERROR : player=" + player->getNick()
                        + " could not drop creature in handon tile "
                        + Ogre::StringConverter::toString(tile->getX())
                        + "," + Ogre::StringConverter::toString(tile->getY()));
                }
            }
            else
            {
                LogManager::getSingleton().logMessage("ERROR : player " + player->getNick()
                    + " could not drop creature in hand on tile "
                    + Ogre::StringConverter::toString(tmpTile.getX())
                    + "," + Ogre::StringConverter::toString(tmpTile.getY()));
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
                try
                {
                    ServerNotification *serverNotification = new ServerNotification(
                        ServerNotification::markTiles, player);
                    int nbTiles = tiles.size();
                    serverNotification->packet << isDigSet << nbTiles;
                    for(std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end(); ++it)
                    {
                        Tile* tile = *it;
                        serverNotification->packet << tile;
                        // We also update the server game map
                        tile->setMarkedForDigging(isDigSet, player);
                    }
                    queueServerNotification(serverNotification);
                }
                catch (std::bad_alloc&)
                {
                    Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in ODServer::processClientNotifications", Ogre::LML_CRITICAL);
                    exit(1);
                }
            }
            break;
        }

        case ClientNotification::askBuildRoom:
        {
            int x1, y1, x2, y2;
            int intType;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2 >> intType);
            std::vector<Tile*> tiles = gameMap->getBuildableTilesForPlayerInArea(x1,
                y1, x2, y2, clientSocket->getPlayer());

            if(!tiles.empty())
            {
                Room::RoomType type = static_cast<Room::RoomType>(intType);
                int goldRequired = tiles.size() * Room::costPerTile(type);
                if(gameMap->withdrawFromTreasuries(goldRequired, clientSocket->getPlayer()->getSeat()))
                    gameMap->buildRoomForPlayer(tiles, type, clientSocket->getPlayer());
            }
            break;
        }

        case ClientNotification::askBuildTrap:
        {
            int x1, y1, x2, y2;
            int intType;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2 >> intType);
            std::vector<Tile*> tiles = gameMap->getBuildableTilesForPlayerInArea(x1,
                y1, x2, y2, clientSocket->getPlayer());

            if(!tiles.empty())
            {
                Trap::TrapType type = static_cast<Trap::TrapType>(intType);
                int goldRequired = tiles.size() * Trap::costPerTile(type);
                if(gameMap->withdrawFromTreasuries(goldRequired, clientSocket->getPlayer()->getSeat()))
                    gameMap->buildTrapForPlayer(tiles, type, clientSocket->getPlayer());
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
            serverNotification->packet << msg;
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
