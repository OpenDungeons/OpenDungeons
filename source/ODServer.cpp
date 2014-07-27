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
#include "ODFrameListener.h"
#include "MapLight.h"
#include "RenderManager.h"
#include "LogManager.h"

#include <SFML/Network.hpp>
#include <SFML/System.hpp>

template<> ODServer* Ogre::Singleton<ODServer>::msSingleton = 0;

ODServer::ODServer() :
    mNbClientsNotReady(0)
{

}

ODServer::~ODServer()
{

}

bool ODServer::startServer(std::string& levelFilename, bool replaceHumanPlayersByAi)
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();

    LogManager& logManager = LogManager::getSingleton();

    mLevelFilename = levelFilename;

    // Start the server socket listener as well as the server socket thread
    if (ODServer::getSingleton().isConnected())
    {
        logManager.logMessage("Couldn't start server: The server is already connected");
        return false;
    }
    if (ODClient::getSingleton().isConnected())
    {
        logManager.logMessage("Couldn't start server: The client is already connected");
        return false;
    }

    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    if (gameMap == NULL)
        return false;

    // Read in the map. The map loading should be happen here and not in the server thread to
    // make sure it is valid before launching the server.
    if (!gameMap->LoadLevel(levelFilename))
        return false;

    // Fill seats with either player, AIs or nothing depending on the given faction.
    uint32_t i = 0;
    uint32_t uniqueAINumber = 1;
    uint32_t nbPlayerSeat = 0;
    mNbClientsNotReady = 0;
    while (i < gameMap->numEmptySeats())
    {
        Seat* seat = gameMap->getEmptySeat(i);

        if (seat->mFaction == "Player")
        {
            ++nbPlayerSeat;
            // Add local player on first slot available.
            if (gameMap->getLocalPlayer()->getSeat() == NULL)
            {
                // The empty seat is removed, so we loop without incrementing i
                gameMap->getLocalPlayer()->setSeat(gameMap->popEmptySeat(seat->getColor()));
                logManager.logMessage("Adding local player with color="
                    + Ogre::StringConverter::toString(seat->getColor()));
                continue;
            }
            else if(replaceHumanPlayersByAi)
            {
                // NOTE - AI should later have definable names maybe?.
                Player* aiPlayer = new Player();
                aiPlayer->setNick("Keeper AI " + Ogre::StringConverter::toString(uniqueAINumber++));

                // The empty seat is removed by addPlayer(), so we loop without incrementing i
                if (gameMap->addPlayer(aiPlayer, gameMap->popEmptySeat(seat->getColor())))
                {
                    gameMap->assignAI(*aiPlayer, "KeeperAI");
                    continue;
                }
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
            aiPlayer->setNick("Keeper AI " + Ogre::StringConverter::toString(uniqueAINumber++));

            // The empty seat is removed by addPlayer(), so we loop without incrementing i
            if (gameMap->addPlayer(aiPlayer, gameMap->popEmptySeat(seat->getColor())))
            {
                gameMap->assignAI(*aiPlayer, "KeeperAI");
                continue;
            }
        }
        ++i;
    }

    logManager.logMessage("Added: " + Ogre::StringConverter::toString(nbPlayerSeat) + " Human players");
    logManager.logMessage("Added: " + Ogre::StringConverter::toString(uniqueAINumber - 1) + " AI players");

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

void ODServer::startNewTurn(const Ogre::FrameEvent& evt)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
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
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
    catch (std::bad_alloc&)
    {
        LogManager::getSingletonPtr()->logMessage("ERROR:  bad alloc in turnStarted", Ogre::LML_CRITICAL);
        exit(1);
    }

    gameMap->doTurn();
    gameMap->doPlayerAITurn(evt.timeSinceLastFrame);
}

void ODServer::processServerEvents()
{
    doTask(10);
    processServerNotifications();

}

void ODServer::processServerNotifications()
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();

    if(gameMap->getTurnNumber() == -1)
    {
        if(mNbClientsNotReady == 0)
        {
            // Every client is connected and ready, we can launch the game
            try
            {
                // Send turn 0 to init the map
                ServerNotification* serverNotification = new ServerNotification(
                    ServerNotification::turnStarted, NULL);
                serverNotification->packet << static_cast<int64_t>(0);
                ODServer::getSingleton().queueServerNotification(serverNotification);
            }
            catch (std::bad_alloc&)
            {
                LogManager::getSingleton().logMessage("ERROR: bad alloc in turnStarted", Ogre::LML_CRITICAL);
                exit(1);
            }
            gameMap->setTurnNumber(0);
            gameMap->setGamePaused(false);
        }
        else
        {
            // We are still waiting for players
            return;
        }
    }

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

            case ServerNotification::animatedObjectAddDestination:
                sendToAllClients(event->packet);
                break;

            case ServerNotification::animatedObjectClearDestinations:
                sendToAllClients(event->packet);
                break;

                //NOTE: this code is duplicated in clientNotificationProcessor
            case ServerNotification::creaturePickedUp:
                sendToAllClients(event->packet);
                break;

            case ServerNotification::creatureDropped:
                sendToAllClients(event->packet);
                break;

            case ServerNotification::setObjectAnimationState:
                sendToAllClients(event->packet);
                break;

            case ServerNotification::setTurnsPerSecond:
                // This one is not used on client side. Shall we remove it?
                sendToAllClients(event->packet);
                break;

            case ServerNotification::tileFullnessChange:
                sendToAllClients(event->packet);
                break;

            case ServerNotification::addMapLight:
                sendToAllClients(event->packet);
                break;

            case ServerNotification::removeMapLight:
                sendToAllClients(event->packet);
                break;

            case ServerNotification::addCreature:
                sendToAllClients(event->packet);
                break;

            case ServerNotification::tileClaimed:
                sendToAllClients(event->packet);
                break;

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

            case ServerNotification::addCreatureBed:
                sendToAllClients(event->packet);
                break;

            case ServerNotification::buildRoom:
                sendToAllClients(event->packet);
                break;

            case ServerNotification::buildTrap:
                sendToAllClients(event->packet);
                break;

            case ServerNotification::removeCreatureBed:
                sendToAllClients(event->packet);
                break;

            case ServerNotification::createTreasuryIndicator:
                sendToAllClients(event->packet);
                break;

            case ServerNotification::destroyTreasuryIndicator:
                sendToAllClients(event->packet);
                break;

            case ServerNotification::exit:
                running = false;
                stopServer();
                break;

            default:
                LogManager::getSingleton().logMessage("ERROR: Unhandled ServerNotification type encoutered="
                    + Ogre::StringConverter::toString(static_cast<int32_t>(event->mType)));
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

    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    GameMap* gameMap = frameListener->getGameMap();
    if (!gameMap)
        return true;

    std::string clientCommand;
    ODPacket packetReceived;

    ODSocketClient::ODComStatus status = receiveMsgFromClient(clientSocket, packetReceived);

    // If the client closed the connection
    if (status != ODSocketClient::ODComStatus::OK)
    {
        return (status != ODSocketClient::ODComStatus::Error);
    }

    OD_ASSERT_TRUE(packetReceived >> clientCommand);

    if (clientCommand.compare(ClientNotification::typeString(ClientNotification::hello)) == 0)
    {
        ODPacket packetSend;
        std::string version;
        std::string levelFilename;
        OD_ASSERT_TRUE(packetReceived >> version >> levelFilename);
        frameListener->addChatMessage(new ChatMessage(
                "SERVER_INFORMATION: ", "Client connect with version: "
                        + version + ", levelFilename=" + levelFilename, time(NULL)));

        // If the levelFilename is different, we refuse the client
        if(levelFilename.compare(mLevelFilename) != 0)
        {
            LogManager::getSingleton().logMessage("Server rejected client awaited map= "
                + mLevelFilename + ", received=" + levelFilename);
            return false;
        }

        // Tell the client to give us their nickname and to clear their map
        packetSend.clear();
        packetSend << ServerNotification::typeString(ServerNotification::pickNick);
        sendMsgToClient(clientSocket, packetSend);

        packetReceived.clear();
        receiveMsgFromClient(clientSocket, packetReceived);

        // Pick nick
        std::string clientCmd;
        std::string clientNick;
        OD_ASSERT_TRUE(packetReceived >> clientCmd >> clientNick);
        OD_ASSERT_TRUE(std::string(ClientNotification::typeString(ClientNotification::setNick)).compare(clientCmd) == 0);

        frameListener->addChatMessage(new ChatMessage(
                "SERVER_INFORMATION: ", "Client nick is: " + clientNick,
                time(NULL)));

        // Create a player structure for the client
        // TODO:  negotiate and set a color
        Player *curPlayer = new Player;
        curPlayer->setNick(clientNick);
        // The seat should be available since it has been checked before accepting the client connexion
        Seat* seat = gameMap->getEmptySeat("Player");
        if(seat != NULL)
        {
            gameMap->addPlayer(curPlayer, gameMap->popEmptySeat(seat->getColor()));
        }
        else
        {
            // No seat available. We disconnect the client
            delete curPlayer;
            return false;
        }

        packetSend.clear();
        packetSend << ServerNotification::typeString(ServerNotification::yourSeat) << seat->getColor();
        sendMsgToClient(clientSocket, packetSend);

        packetSend.clear();
        packetSend << ServerNotification::typeString(ServerNotification::turnsPerSecond) << ODApplication::turnsPerSecond;
        sendMsgToClient(clientSocket, packetSend);

        // Send over the information the game. Clients can connect only before
        // a game is launched (it would be too long to transfer all data otherwise).
        // So we assume that they have loaded the same map (we have tested it as well
        // as we could previously). We just need to send them information about the
        // players
        packetSend.clear();
        packetSend << ServerNotification::typeString(ServerNotification::addPlayer) << gameMap->getLocalPlayer()->getNick()
            << gameMap->getLocalPlayer()->getSeat()->getColor();
        sendMsgToClient(clientSocket, packetSend);
        for (unsigned int i = 0; i < gameMap->numPlayers(); ++i)
        {
            // Don't tell the client about its own player structure
            Player* tempPlayer = gameMap->getPlayer(i);
            if (curPlayer != tempPlayer && tempPlayer != NULL)
            {
                packetSend.clear();
                packetSend << ServerNotification::typeString(ServerNotification::addPlayer) << tempPlayer->getNick()
                    << tempPlayer->getSeat()->getColor();
                sendMsgToClient(clientSocket, packetSend);
            }
        }

         // The player received everything. He is ready
         clientSocket->setPlayer(curPlayer);
        --mNbClientsNotReady;
        LogManager::getSingleton().logMessage("Player=" + curPlayer->getNick()
            + " has been accepted in the game on color="
            + Ogre::StringConverter::toString(curPlayer->getSeat()->getColor()));
    }

    else if (clientCommand.compare(ClientNotification::typeString(ClientNotification::chat)) == 0)
    {
        std::string chatNick;
        std::string chatMsg;
        OD_ASSERT_TRUE(packetReceived >> chatNick >> chatMsg);
        ChatMessage *newMessage = new ChatMessage(chatNick, chatMsg, time(NULL));

        ODPacket packetSend;
        packetSend << ServerNotification::typeString(ServerNotification::chat) << newMessage->getClientNick() << newMessage->getMessage();
        sendToAllClients(packetSend);

        // Put the message in our own queue
        frameListener->addChatMessage(newMessage);
    }

    else if (clientCommand.compare(ClientNotification::typeString(ClientNotification::askCreaturePickUp)) == 0)
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
    }

    else if (clientCommand.compare(ClientNotification::typeString(ClientNotification::askCreatureDrop)) == 0)
    {
        Tile tmpTile;
        OD_ASSERT_TRUE(packetReceived >> &tmpTile);
        Player *player = clientSocket->getPlayer();
        Tile* tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
        if(tile != NULL)
        {
            if(player->isDropCreaturePossible(tile))
            {
                player->dropCreature(tile);
                SoundEffectsHelper::getSingleton().playInterfaceSound(SoundEffectsHelper::DROP);
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
    }

    else if (clientCommand.compare(ClientNotification::typeString(ClientNotification::askMarkTile)) == 0)
    {
        int x1, y1, x2, y2;
        bool isDigSet;

        OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2 >> isDigSet);
        std::vector<Tile*> tiles = gameMap->getDiggableTilesForPlayerInArea(x1,
            y1, x2, y2, clientSocket->getPlayer());
        if(!tiles.empty())
        {
            // We send to the client the list of tiles to mark. We send to him only
            ODPacket packetSend;
            packetSend << ServerNotification::typeString(ServerNotification::markTiles);
            packetSend << isDigSet << tiles.size();
            for(std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end(); ++it)
            {
                Tile* tile = *it;
                packetSend << tile;
                // We also update the server game map
                tile->setMarkedForDigging(isDigSet, clientSocket->getPlayer());
            }
            sendMsgToClient(clientSocket, packetSend);
        }
    }

    else if (clientCommand.compare(ClientNotification::typeString(ClientNotification::askBuildRoom)) == 0)
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
    }

    else if (clientCommand.compare(ClientNotification::typeString(ClientNotification::askBuildTrap)) == 0)
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
    }

    else if (clientCommand.compare(ClientNotification::typeString(ClientNotification::ackNewTurn)) == 0)
    {
        int64_t turn;
        OD_ASSERT_TRUE(packetReceived >> turn);
        clientSocket->setLastTurnAck(turn);
    }

    else
    {
        LogManager::getSingleton().logMessage("ERROR:  Unhandled command received from client:" + clientCommand);
    }

    return true;
}

void ODServer::sendToAllClients(ODPacket& packetSend)
{
    // TODO : except the console, nothing should decide to whom a cmd should be sent. Check if it is true
    sendMsgToAllClients(packetSend);
}

bool ODServer::notifyNewConnection(ODSocketClient *sock)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    if (gameMap == NULL)
        return false;

    // TODO : the seat should be popped and saved in the client so that we can keep track if several clients
    // connects at same time (otherwise, clients could be accepted even if no seat available)
    Seat* seat = gameMap->getEmptySeat("Player");
    return (seat != NULL);
}

bool ODServer::notifyClientMessage(ODSocketClient *client)
{
    bool ret = processClientNotifications(client);
    if(!ret)
    {
        // TODO : send a message to every client
        ODFrameListener::getSingletonPtr()->addChatMessage(new ChatMessage("SERVER_INFORMATION: ",
            "Client disconnected", time(NULL)));
        // TODO : wait at least 1 minute if the client reconnects
    }
    return ret;
}

void ODServer::stopServer()
{
    // We start by stopping server to make sure no new message comes
    ODSocketServer::stopServer();

    mNbClientsNotReady = 0;

    // Then we proceed the message queue in case it is not empty
    // If the server had his own thread, we would have to wait for him to finish here
    processServerNotifications();
    RenderManager::getSingleton().processRenderRequests();
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    gameMap->clearAll();
}

void ODServer::notifyExit()
{
    while(!mServerNotificationQueue.empty())
    {
        delete mServerNotificationQueue.front();
        mServerNotificationQueue.pop_front();
    }

    ServerNotification* exitServerNotification = new ServerNotification(
        ServerNotification::exit, NULL);
    queueServerNotification(exitServerNotification);
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
