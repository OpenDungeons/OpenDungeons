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

#include "network/ODServer.h"

#include "network/ODClient.h"
#include "network/ServerNotification.h"
#include "gamemap/GameMap.h"
#include "ODApplication.h"
#include "entities/MapLight.h"
#include "network/ChatMessage.h"
#include "modes/ODConsoleCommand.h"
#include "gamemap/MapLoader.h"
#include "entities/RenderedMovableEntity.h"
#include "utils/LogManager.h"
#include "entities/Creature.h"
#include "traps/Trap.h"
#include "traps/TrapCannon.h"
#include "traps/TrapSpike.h"
#include "traps/TrapBoulder.h"
#include "rooms/RoomCrypt.h"
#include "rooms/RoomDormitory.h"
#include "rooms/RoomDungeonTemple.h"
#include "rooms/RoomForge.h"
#include "rooms/RoomHatchery.h"
#include "rooms/RoomLibrary.h"
#include "rooms/RoomPortal.h"
#include "rooms/RoomTrainingHall.h"
#include "rooms/RoomTreasury.h"
#include "utils/ConfigManager.h"

#include <SFML/Network.hpp>
#include <SFML/System.hpp>

#include "boost/filesystem.hpp"

const std::string ODServer::SERVER_INFORMATION = "SERVER_INFORMATION";

template<> ODServer* Ogre::Singleton<ODServer>::msSingleton = 0;

ODServer::ODServer() :
    mServerMode(ServerMode::ModeNone),
    mServerState(ServerState::StateNone),
    mGameMap(new GameMap(true)),
    mSeatsConfigured(false)
{
}

ODServer::~ODServer()
{
    delete mGameMap;
}

bool ODServer::startServer(const std::string& levelFilename, ServerMode mode)
{
    LogManager& logManager = LogManager::getSingleton();
    logManager.logMessage("Asked to launch server with levelFilename=" + levelFilename);

    mLevelFilename = levelFilename;
    mSeatsConfigured = false;

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
    mServerMode = mode;
    mServerState = ServerState::StateConfiguration;
    GameMap* gameMap = mGameMap;
    if (!gameMap->loadLevel(levelFilename))
    {
        mServerMode = ServerMode::ModeNone;
        mServerState = ServerState::StateNone;
        logManager.logMessage("Couldn't start server. The level file can't be loaded: " + levelFilename);
        return false;
    }

    // Set up the socket to listen on the specified port
    if (!createServer(ConfigManager::getSingleton().getNetworkPort()))
    {
        mServerMode = ServerMode::ModeNone;
        mServerState = ServerState::StateNone;
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

void ODServer::sendAsyncMsgToAllClients(ServerNotification& notif)
{
    sendMsgToAllClients(notif.mPacket);
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
    for (ODSocketClient* client : mSockClients)
    {
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
            if(creature == nullptr)
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

    switch(mServerMode)
    {
        case ServerMode::ModeGameSinglePlayer:
        case ServerMode::ModeGameMultiPlayer:
        {
            gameMap->doTurn();
            gameMap->doPlayerAITurn(timeSinceLastFrame);
            break;
        }
        case ServerMode::ModeEditor:
            // We do not update turn in editor mode to not have creatures wander
            break;
        case ServerMode::ModeNone:
            // It is not normal to have no mode selected and starting turns
            OD_ASSERT_TRUE(false);
            break;
    }

    gameMap->processDeletionQueues();
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
            if(mSeatsConfigured)
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
                for(Seat* seat : gameMap->getSeats())
                {
                    if(seat->getPlayer() == nullptr)
                        continue;

                    if(seat->getStartingGold() > 0)
                        gameMap->addGoldToSeat(seat->getStartingGold(), seat->getId());
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
        // creatures going through walls.
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
                sendMsgToAllClients(event->mPacket);
                break;

            case ServerNotification::setTurnsPerSecond:
                // This one is not used on client side. Shall we remove it?
                sendMsgToAllClients(event->mPacket);
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

            case ServerNotification::playerFighting:
            {
                ODSocketClient* client = getClientFromPlayer(event->mConcernedPlayer);
                OD_ASSERT_TRUE_MSG(client != NULL, "name=" + event->mConcernedPlayer->getNick());
                if(client != NULL)
                    sendMsgToClient(client, event->mPacket);
                break;
            }

            case ServerNotification::playerNoMoreFighting:
            {
                ODSocketClient* client = getClientFromPlayer(event->mConcernedPlayer);
                OD_ASSERT_TRUE_MSG(client != NULL, "name=" + event->mConcernedPlayer->getNick());
                if(client != NULL)
                    sendMsgToClient(client, event->mPacket);
                break;
            }

            case ServerNotification::entityPickedUp:
                // This message should not be sent by human players (they are notified asynchronously)
                OD_ASSERT_TRUE_MSG(!event->mConcernedPlayer->getIsHuman(), "nick=" + event->mConcernedPlayer->getNick());
                sendMsgToAllClients(event->mPacket);
                break;

            case ServerNotification::entityDropped:
                // This message should not be sent by human players (they are notified asynchronously)
                OD_ASSERT_TRUE_MSG(!event->mConcernedPlayer->getIsHuman(), "nick=" + event->mConcernedPlayer->getNick());
                sendMsgToAllClients(event->mPacket);
                break;

            case ServerNotification::entitySlapped:
                // This message should not be sent by human players (they are notified asynchronously)
                OD_ASSERT_TRUE_MSG(!event->mConcernedPlayer->getIsHuman(), "nick=" + event->mConcernedPlayer->getNick());
                sendMsgToAllClients(event->mPacket);
                break;

            case ServerNotification::buildRoom:
            {
                // This message should not be sent by human players (they are notified asynchronously)
                OD_ASSERT_TRUE_MSG(!event->mConcernedPlayer->getIsHuman(), "nick=" + event->mConcernedPlayer->getNick());
                sendMsgToAllClients(event->mPacket);
                break;
            }

            case ServerNotification::playerWon:
            {
                if(!event->mConcernedPlayer->getIsHuman())
                    break;

                ODSocketClient* client = getClientFromPlayer(event->mConcernedPlayer);
                OD_ASSERT_TRUE_MSG(client != NULL, "name=" + event->mConcernedPlayer->getNick());
                if(client != NULL)
                {
                    ODPacket packet;
                    packet << ServerNotification::chatServer;
                    packet << "You Won";
                    sendMsgToClient(client, packet);
                }
                break;
            }

            case ServerNotification::playerLost:
            {
                // We check if there is still a player in the team with a dungeon temple. If yes, we notify the player he lost his dungeon
                // if no, we notify the team they lost
                std::vector<Room*> dungeonTemples = gameMap->getRoomsByType(Room::RoomType::dungeonTemple);
                bool hasTeamLost = true;
                for(std::vector<Room*>::iterator it = dungeonTemples.begin(); it != dungeonTemples.end(); ++it)
                {
                    Room* dungeonTemple = *it;
                    if(event->mConcernedPlayer->getSeat()->isAlliedSeat(dungeonTemple->getSeat()))
                    {
                        hasTeamLost = false;
                        break;
                    }
                }

                if(hasTeamLost)
                {
                    // This message will be sent in skirmish or multiplayer so it should not talk about team. If we want to be
                    // more precise, we shall handle the case
                    ODPacket packet;
                    packet << ServerNotification::chatServer;
                    packet << "You lost the game";
                    for (std::vector<ODSocketClient*>::iterator it = mSockClients.begin(); it != mSockClients.end(); ++it)
                    {
                        ODSocketClient* client = *it;
                        if(!event->mConcernedPlayer->getSeat()->isAlliedSeat(client->getPlayer()->getSeat()))
                            continue;

                        sendMsgToClient(client, packet);
                    }
                    break;
                }

                ODPacket packetAllied;
                packetAllied << ServerNotification::chatServer;
                packetAllied << "An ally has lost";
                for (std::vector<ODSocketClient*>::iterator it = mSockClients.begin(); it != mSockClients.end(); ++it)
                {
                    ODSocketClient* client = *it;
                    if(!event->mConcernedPlayer->getSeat()->isAlliedSeat(client->getPlayer()->getSeat()))
                            continue;

                    if(client->getPlayer() != event->mConcernedPlayer)
                    {
                        sendMsgToClient(client, packetAllied);
                    }
                    else
                    {
                        ODPacket packet;
                        packet << ServerNotification::chatServer;
                        packet << "You lost";
                        sendMsgToClient(client, packet);
                    }
                }
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
                sendMsgToAllClients(event->mPacket);
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
        // If a client disconnects during seat configuration, we delete him from the list
        if(mServerState != ServerState::StateConfiguration)
            return (status != ODSocketClient::ODComStatus::Error);

        uint32_t nbPlayers = 1;
        ODPacket packetSend;
        packetSend << ServerNotification::removePlayers << nbPlayers;
        int32_t id = clientSocket->getPlayer()->getId();
        packetSend << id;
        sendMsgToAllClients(packetSend);
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
            LogManager::getSingleton().logMessage("Level relative path sent to client: " + mLevelFilename);
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
            packetSend << ServerNotification::pickNick << mServerMode;
            sendMsgToClient(clientSocket, packetSend);
            break;
        }

        case ClientNotification::setNick:
        {
            if(std::string("nick").compare(clientSocket->getState()) != 0)
                return false;

            // Pick nick
            std::string clientNick;
            OD_ASSERT_TRUE(packetReceived >> clientNick);

            // NOTE : playerId 0 is reserved for inactive players and 1 is reserved for AI
            int32_t playerId = mSockClients.size() + 10;
            Player* curPlayer = new Player(gameMap, playerId);
            curPlayer->setNick(clientNick);
            curPlayer->setIsHuman(true);
            clientSocket->setPlayer(curPlayer);
            setClientState(clientSocket, "ready");

            if(mServerMode != ServerMode::ModeEditor)
                break;

            // On editor mode, we configure automatically seats
            mServerState = ServerState::StateGame;
            const std::vector<Seat*>& seats = gameMap->getSeats();
            OD_ASSERT_TRUE(!seats.empty());
            if(seats.empty())
                break;

            Seat* seat = seats[0];
            seat->setPlayer(curPlayer);
            ODPacket packetSend;
            packetSend << ServerNotification::clientAccepted << ODApplication::turnsPerSecond;
            int32_t nbPlayers = 1;
            packetSend << nbPlayers;
            const std::string& nick = clientSocket->getPlayer()->getNick();
            int32_t id = clientSocket->getPlayer()->getId();
            int seatId = seat->getId();
            packetSend << nick << id << seatId;
            sendMsgToClient(clientSocket, packetSend);

            packetSend.clear();
            packetSend << ServerNotification::startGameMode << seatId << mServerMode;
            sendMsgToClient(clientSocket, packetSend);
            mSeatsConfigured = true;
            break;
        }

        case ClientNotification::readyForSeatConfiguration:
        {
            if(std::string("ready").compare(clientSocket->getState()) != 0)
                return false;

            ODPacket packetSend;
            // We notify to the newly connected player all the currently connected players (including himself
            uint32_t nbPlayers = mSockClients.size();
            packetSend << ServerNotification::addPlayers << nbPlayers;
            for (ODSocketClient* client : mSockClients)
            {
                const std::string& nick = client->getPlayer()->getNick();
                int32_t id = client->getPlayer()->getId();
                packetSend << nick << id;
            }
            sendMsgToClient(clientSocket, packetSend);

            // Then, we notify the newly connected client to every client
            const std::string& clientNick = clientSocket->getPlayer()->getNick();
            int32_t clientPlayerId = clientSocket->getPlayer()->getId();
            packetSend.clear();
            nbPlayers = 1;
            packetSend << ServerNotification::addPlayers << nbPlayers;
            packetSend << clientNick << clientPlayerId;
            for (ODSocketClient* client : mSockClients)
            {
                if(clientSocket == client)
                    continue;

                sendMsgToClient(client, packetSend);
            }
            break;
        }

        case ClientNotification::seatConfigurationRefresh:
        {
            std::vector<Seat*> seats = gameMap->getSeats();
            ODPacket packetSend;
            packetSend << ServerNotification::seatConfigurationRefresh;
            for(Seat* seat : seats)
            {
                int seatId;
                bool isSet;
                OD_ASSERT_TRUE(packetReceived >> seatId);
                OD_ASSERT_TRUE(seatId == seat->getId());
                packetSend << seatId;

                OD_ASSERT_TRUE(packetReceived >> isSet);
                packetSend << isSet;
                if(isSet)
                {
                    uint32_t factionIndex;
                    OD_ASSERT_TRUE(packetReceived >> factionIndex);
                    packetSend << factionIndex;
                }
                OD_ASSERT_TRUE(packetReceived >> isSet);
                packetSend << isSet;
                if(isSet)
                {
                    int32_t playerId;
                    OD_ASSERT_TRUE(packetReceived >> playerId);
                    packetSend << playerId;
                }
                OD_ASSERT_TRUE(packetReceived >> isSet);
                packetSend << isSet;
                if(isSet)
                {
                    int32_t teamId;
                    OD_ASSERT_TRUE(packetReceived >> teamId);
                    packetSend << teamId;
                }
            }
            sendMsgToAllClients(packetSend);
            break;
        }

        case ClientNotification::seatConfigurationSet:
        {
            // We change server state to make sure no new client will be accepted
            OD_ASSERT_TRUE_MSG(mServerState == ServerState::StateConfiguration, "Wrong server state="
                + Ogre::StringConverter::toString(static_cast<int>(mServerState)));
            mServerState = ServerState::StateGame;

            std::vector<Seat*> seats = gameMap->getSeats();
            const std::vector<std::string>& factions = ConfigManager::getSingleton().getFactions();
            for(Seat* seat : seats)
            {
                int seatId;
                bool isSet;
                uint32_t factionIndex;
                int32_t playerId;
                int32_t teamId;
                OD_ASSERT_TRUE(packetReceived >> seatId);
                OD_ASSERT_TRUE(seatId == seat->getId());
                OD_ASSERT_TRUE(packetReceived >> isSet);
                // It is not acceptable to have an incompletely configured seat
                OD_ASSERT_TRUE(isSet);
                OD_ASSERT_TRUE(packetReceived >> factionIndex);
                OD_ASSERT_TRUE(factionIndex < factions.size());
                seat->setFaction(factions[factionIndex]);

                OD_ASSERT_TRUE(packetReceived >> isSet);
                // It is not acceptable to have an incompletely configured seat
                OD_ASSERT_TRUE(isSet);
                OD_ASSERT_TRUE(packetReceived >> playerId);
                switch(playerId)
                {
                    case 0:
                    {
                        // It is an inactive player
                        break;
                    }
                    case 1:
                    {
                        // It is an AI
                        // We set player id = 0 for AI players. ID is only used during seat configuration phase
                        // During the game, one should use the seat ID to identify a player
                        Player* aiPlayer = new Player(gameMap, 0);
                        aiPlayer->setNick("Keeper AI " + Ogre::StringConverter::toString(seatId));
                        gameMap->addPlayer(aiPlayer);
                        seat->setPlayer(aiPlayer);
                        gameMap->assignAI(*aiPlayer, "KeeperAI");
                        break;
                    }
                    default:
                    {
                        // Human player
                        for (ODSocketClient* client : mSockClients)
                        {
                            if((client->getState().compare("ready") == 0) &&
                               (client->getPlayer()->getId() == playerId))
                            {
                                seat->setPlayer(client->getPlayer());
                                gameMap->addPlayer(client->getPlayer());
                                break;
                            }
                        }
                        break;
                    }
                }

                OD_ASSERT_TRUE(packetReceived >> isSet);
                // It is not acceptable to have an incompletely configured seat
                OD_ASSERT_TRUE(isSet);
                OD_ASSERT_TRUE(packetReceived >> teamId);
                seat->setTeamId(teamId);

                // If a player is assigned to this seat, we create his spawn pool
                if(seat->getPlayer() != nullptr)
                    seat->initSpawnPool();
                else
                    LogManager::getSingleton().logMessage("No spawn pool created for seat id="
                        + Ogre::StringConverter::toString(seat->getId()));
            }

            // Now, we can disconnect the players that were not configured
            std::vector<ODSocketClient*> clientsToRemove;
            for (ODSocketClient* client : mSockClients)
            {
                if(client->getPlayer()->getSeat() == nullptr)
                    clientsToRemove.push_back(client);
            }

            if(!clientsToRemove.empty())
            {
                ODPacket packetSend;
                packetSend << ServerNotification::clientRejected;
                for(ODSocketClient* client : clientsToRemove)
                {
                    Player* player = client->getPlayer();
                    LogManager::getSingleton().logMessage("Rejecting player id="
                        + Ogre::StringConverter::toString(player->getId())
                        + ", nick=" + player->getNick());
                    setClientState(client, "rejected");
                    sendMsgToClient(client, packetSend);
                    delete player;
                    client->setPlayer(nullptr);
                }
            }

            ODPacket packetSend;
            packetSend << ServerNotification::clientAccepted << ODApplication::turnsPerSecond;
            const std::vector<Player*>& players = gameMap->getPlayers();
            int32_t nbPlayers = players.size();
            packetSend << nbPlayers;
            for (Player* player : players)
            {
                packetSend << player->getNick() << player->getId() << player->getSeat()->getId();
            }
            sendMsgToAllClients(packetSend);

            for (ODSocketClient* client : mSockClients)
            {
                if(!client->isConnected() || (client->getPlayer() == nullptr))
                    continue;

                ODPacket packetSend;
                int seatId = client->getPlayer()->getSeat()->getId();
                packetSend << ServerNotification::startGameMode << seatId << mServerMode;
                sendMsgToClient(client, packetSend);
            }

            mSeatsConfigured = true;
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
            ServerNotification notif(ServerNotification::chat, player);
            notif.mPacket << nick << chatMsg;
            sendAsyncMsgToAllClients(notif);
            break;
        }

        case ClientNotification::askEntityPickUp:
        {
            std::string entityName;
            GameEntity::ObjectType entityType;
            OD_ASSERT_TRUE(packetReceived >> entityType >> entityName);

            Player *player = clientSocket->getPlayer();
            GameEntity* entity = gameMap->getEntityFromTypeAndName(entityType, entityName);
            OD_ASSERT_TRUE_MSG(entity != nullptr, "entityType=" + Ogre::StringConverter::toString(static_cast<int32_t>(entityType)) + ", entityName=" + entityName);
            if(entity == nullptr)
                break;
            bool allowPickup = entity->tryPickup(player->getSeat(), mServerMode == ServerMode::ModeEditor);
            if(!allowPickup)
            {
                LogManager::getSingleton().logMessage("player=" + player->getNick()
                        + " could not pickup entity entityType="
                        + Ogre::StringConverter::toString(static_cast<int32_t>(entityType))
                        + ", entityName=" + entityName);
                break;
            }

            player->pickUpEntity(entity, mServerMode == ServerMode::ModeEditor);
            break;
        }

        case ClientNotification::askHandDrop:
        {
            Tile tmpTile(gameMap);
            OD_ASSERT_TRUE(packetReceived >> &tmpTile);
            Player *player = clientSocket->getPlayer();
            Tile* tile = gameMap->getTile(tmpTile.getX(), tmpTile.getY());
            OD_ASSERT_TRUE_MSG(tile != nullptr, "tile=" + Tile::displayAsString(&tmpTile));
            if(tile != nullptr)
            {
                if(player->isDropHandPossible(tile, 0, mServerMode == ServerMode::ModeEditor))
                {
                    // We notify the players
                    OD_ASSERT_TRUE(player->dropHand(tile, 0) != NULL);
                }
                else
                {
                    LogManager::getSingleton().logMessage("player=" + player->getNick()
                        + " could not drop entity in hand on tile "
                        + Tile::displayAsString(tile));
                }
            }
            break;
        }

        case ClientNotification::askPickupWorker:
        {
            Player *player = clientSocket->getPlayer();
            Creature* creature = gameMap->getWorkerToPickupBySeat(player->getSeat());
            if(creature == nullptr)
                break;

            player->pickUpEntity(creature, mServerMode == ServerMode::ModeEditor);
            break;
        }

        case ClientNotification::askPickupFighter:
        {
            Player *player = clientSocket->getPlayer();
            Creature* creature = gameMap->getFighterToPickupBySeat(player->getSeat());
            if(creature == nullptr)
                break;

            player->pickUpEntity(creature, mServerMode == ServerMode::ModeEditor);
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

        case ClientNotification::askSlapEntity:
        {
            GameEntity::ObjectType entityType;
            std::string entityName;
            Player* player = clientSocket->getPlayer();
            OD_ASSERT_TRUE(packetReceived >> entityType >> entityName);
            GameEntity* entity = gameMap->getEntityFromTypeAndName(entityType, entityName);
            OD_ASSERT_TRUE_MSG(entity != nullptr, "entityType=" + Ogre::StringConverter::toString(static_cast<int32_t>(entityType)) + ", entityName=" + entityName);
            if(entity == nullptr)
                break;


            bool isEditorMode = (mServerMode == ServerMode::ModeEditor);
            if(!entity->canSlap(player->getSeat(), isEditorMode))
            {
                LogManager::getSingleton().logMessage("player=" + player->getNick()
                        + " could not slap entity entityType="
                        + Ogre::StringConverter::toString(static_cast<int32_t>(entityType))
                        + ", entityName=" + entityName);
                break;
            }

            entity->slap(isEditorMode);

            ODPacket packet;
            int seatId = player->getSeat()->getId();
            packet << ServerNotification::entitySlapped;
            packet << isEditorMode;
            packet << seatId;
            packet << entityType;
            packet << entityName;
            sendMsgToAllClients(packet);
            break;
        }

        case ClientNotification::askBuildRoom:
        {
            int x1, y1, x2, y2;
            Room::RoomType type;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2 >> type);
            Player* player = clientSocket->getPlayer();

            std::vector<Tile*> tiles;
            int goldRequired;
            gameMap->fillBuildableTilesAndPriceForPlayerInArea(x1, y1, x2, y2, player, type, tiles, goldRequired);
            if(tiles.empty())
                break;

            if(gameMap->withdrawFromTreasuries(goldRequired, player->getSeat()) == false)
                break;

            Room* room = nullptr;
            switch (type)
            {
                case Room::RoomType::nullRoomType:
                {
                    room = nullptr;
                    break;
                }
                case Room::RoomType::dormitory:
                {
                    room = new RoomDormitory(gameMap);
                    break;
                }
                case Room::RoomType::dungeonTemple:
                {
                    room = new RoomDungeonTemple(gameMap);
                    break;
                }
                case Room::RoomType::forge:
                {
                    room = new RoomForge(gameMap);
                    break;
                }
                case Room::RoomType::hatchery:
                {
                    room = new RoomHatchery(gameMap);
                    break;
                }
                case Room::RoomType::library:
                {
                    room = new RoomLibrary(gameMap);
                    break;
                }
                case Room::RoomType::portal:
                {
                    room = new RoomPortal(gameMap);
                    break;
                }
                case Room::RoomType::trainingHall:
                {
                    room = new RoomTrainingHall(gameMap);
                    break;
                }
                case Room::RoomType::treasury:
                {
                    room = new RoomTreasury(gameMap);
                    break;
                }
                case Room::RoomType::crypt:
                {
                    room = new RoomCrypt(gameMap);
                    break;
                }
                default:
                    OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                        static_cast<int>(type)));
            }
            if(room == nullptr)
                break;

            room->setupRoom(gameMap->nextUniqueNameRoom(room->getMeshName()), player->getSeat(), tiles);
            gameMap->addRoom(room, true);
            room->createMesh();
            room->checkForRoomAbsorbtion();
            room->updateActiveSpots();
            gameMap->refreshBorderingTilesOf(tiles);
            break;
        }

        case ClientNotification::askSellRoomTiles:
        {
            int x1, y1, x2, y2;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2);
            Player* player = clientSocket->getPlayer();
            std::vector<Tile*> tiles = gameMap->rectangularRegion(x1, y1, x2, y2);

            if(tiles.empty())
                break;

            int goldRetrieved = 0;
            std::set<Room*> roomsImpacted;
            std::vector<Tile*> sentTiles;
            for(Tile* tile : tiles)
            {
                if(tile->getCoveringRoom() == nullptr)
                    continue;

                Room* room = tile->getCoveringRoom();
                if(!room->getSeat()->canRoomBeDestroyedBy(player->getSeat()))
                    continue;

                roomsImpacted.insert(room);
                sentTiles.push_back(tile);
                goldRetrieved += Room::costPerTile(room->getType()) / 2;
                OD_ASSERT_TRUE(room->removeCoveredTile(tile));
                const std::string& name = room->getName();
                ServerNotification notif(ServerNotification::removeRoomTile, player);
                notif.mPacket << name << tile;
                sendAsyncMsgToAllClients(notif);
            }

            // We update active spots of each impacted rooms
            for(std::set<Room*>::iterator it = roomsImpacted.begin(); it != roomsImpacted.end(); ++it)
            {
                Room* room = *it;
                room->updateActiveSpots();
            }

            // Note : no need to handle the free treasury tile because if there is no treasury tile, gold will be 0 anyway
            gameMap->addGoldToSeat(goldRetrieved, player->getSeat()->getId());
            break;
        }

        case ClientNotification::editorAskDestroyRoomTiles:
        {
            OD_ASSERT_TRUE_MSG(mServerMode == ServerMode::ModeEditor, "Received editor command while wrong mode mode"
                + Ogre::StringConverter::toString(static_cast<int>(mServerMode)));
            int x1, y1, x2, y2;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2);
            std::vector<Tile*> tiles = gameMap->rectangularRegion(x1, y1, x2, y2);

            if(tiles.empty())
                break;

            std::set<Room*> roomsImpacted;
            std::vector<Tile*> sentTiles;
            for(Tile* tile : tiles)
            {
                if(tile->getCoveringRoom() == nullptr)
                    continue;

                Room* room = tile->getCoveringRoom();
                roomsImpacted.insert(room);
                sentTiles.push_back(tile);
                OD_ASSERT_TRUE(room->removeCoveredTile(tile));
                const std::string& name = room->getName();
                ServerNotification notif(ServerNotification::removeRoomTile, clientSocket->getPlayer());
                notif.mPacket << name << tile;
                sendAsyncMsgToAllClients(notif);
            }

            // We update active spots of each impacted rooms
            for(std::set<Room*>::iterator it = roomsImpacted.begin(); it != roomsImpacted.end(); ++it)
            {
                Room* room = *it;
                room->updateActiveSpots();
            }
            break;
        }

        case ClientNotification::askBuildTrap:
        {
            int x1, y1, x2, y2;
            Trap::TrapType type;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2 >> type);
            Player* player = clientSocket->getPlayer();
            std::vector<Tile*> tiles = gameMap->getBuildableTilesForPlayerInArea(x1,
                y1, x2, y2, player);

            if(tiles.empty())
                break;

            int goldRequired = tiles.size() * Trap::costPerTile(type);
            if(!gameMap->withdrawFromTreasuries(goldRequired, player->getSeat()))
                break;

            Trap* trap = nullptr;
            switch (type)
            {
                case Trap::TrapType::nullTrapType:
                {
                    trap = nullptr;
                    break;
                }
                case Trap::TrapType::cannon:
                {
                    trap = new TrapCannon(gameMap);
                    break;
                }
                case Trap::TrapType::spike:
                {
                    trap = new TrapSpike(gameMap);
                    break;
                }
                case Trap::TrapType::boulder:
                {
                    trap = new TrapBoulder(gameMap);
                    break;
                }
                default:
                    OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                        static_cast<int>(type)));
            }
            if(trap == nullptr)
                break;

            trap->setupTrap(gameMap->nextUniqueNameTrap(trap->getMeshName()), player->getSeat(), tiles);
            gameMap->addTrap(trap);
            trap->createMesh();
            trap->updateActiveSpots();
            break;
        }

        case ClientNotification::askSellTrapTiles:
        {
            int x1, y1, x2, y2;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2);
            Player* player = clientSocket->getPlayer();
            std::vector<Tile*> tiles = gameMap->rectangularRegion(x1, y1, x2, y2);

            if(tiles.empty())
                break;

            int goldRetrieved = 0;
            std::set<Trap*> trapsImpacted;
            std::vector<Tile*> sentTiles;
            for(Tile* tile : tiles)
            {
                if(tile->getCoveringTrap() == nullptr)
                    continue;

                Trap* trap = tile->getCoveringTrap();
                if(!trap->getSeat()->canTrapBeDestroyedBy(player->getSeat()))
                    continue;

                trapsImpacted.insert(trap);
                sentTiles.push_back(tile);
                goldRetrieved += Trap::costPerTile(trap->getType()) / 2;
                OD_ASSERT_TRUE(trap->removeCoveredTile(tile));
                const std::string& name = trap->getName();
                ServerNotification notif(ServerNotification::removeTrapTile, player);
                notif.mPacket << name << tile;
                sendAsyncMsgToAllClients(notif);
            }

            // We update active spots of each impacted rooms
            for(std::set<Trap*>::iterator it = trapsImpacted.begin(); it != trapsImpacted.end(); ++it)
            {
                Trap* trap = *it;
                trap->updateActiveSpots();
            }
            gameMap->addGoldToSeat(goldRetrieved, player->getSeat()->getId());
            break;
        }

        case ClientNotification::editorAskDestroyTrapTiles:
        {
            OD_ASSERT_TRUE_MSG(mServerMode == ServerMode::ModeEditor, "Received editor command while wrong mode mode"
                + Ogre::StringConverter::toString(static_cast<int>(mServerMode)));
            int x1, y1, x2, y2;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2);
            std::vector<Tile*> tiles = gameMap->rectangularRegion(x1, y1, x2, y2);

            if(tiles.empty())
                break;

            std::set<Trap*> trapsImpacted;
            std::vector<Tile*> sentTiles;
            for(Tile* tile : tiles)
            {
                if(tile->getCoveringTrap() == nullptr)
                    continue;

                Trap* trap = tile->getCoveringTrap();
                trapsImpacted.insert(trap);
                sentTiles.push_back(tile);
                OD_ASSERT_TRUE(trap->removeCoveredTile(tile));
                const std::string& name = trap->getName();
                ServerNotification notif(ServerNotification::removeTrapTile, clientSocket->getPlayer());
                notif.mPacket << name << tile;
                sendAsyncMsgToAllClients(notif);
            }

            // We update active spots of each impacted rooms
            for(std::set<Trap*>::iterator it = trapsImpacted.begin(); it != trapsImpacted.end(); ++it)
            {
                Trap* trap = *it;
                trap->updateActiveSpots();
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

        case ClientNotification::editorAskSaveMap:
        {
            OD_ASSERT_TRUE_MSG(mServerMode == ServerMode::ModeEditor, "Received editor command while wrong mode mode"
                + Ogre::StringConverter::toString(static_cast<int>(mServerMode)));
            Player* player = clientSocket->getPlayer();
            if(player->numObjectsInHand() == 0)
            {
                // If the file exists, we make a backup
                const boost::filesystem::path levelPath(mLevelFilename);
                if (boost::filesystem::exists(levelPath))
                    boost::filesystem::rename(mLevelFilename, mLevelFilename + ".bak");

                std::string msg = "Map saved successfully";
                MapLoader::writeGameMapToFile(mLevelFilename, *gameMap);
                ServerNotification notif(ServerNotification::chatServer, player);
                notif.mPacket << msg;
                sendAsyncMsgToAllClients(notif);
            }
            else
            {
                // We cannot save the map
                std::string msg = "Map could not be saved because player hand is not empty";
                ServerNotification notif(ServerNotification::chatServer, player);
                notif.mPacket << msg;
                sendAsyncMsgToAllClients(notif);
            }
            break;
        }

        case ClientNotification::editorAskChangeTiles:
        {
            OD_ASSERT_TRUE_MSG(mServerMode == ServerMode::ModeEditor, "Received editor command while wrong mode mode"
                + Ogre::StringConverter::toString(static_cast<int>(mServerMode)));
            int x1, y1, x2, y2;
            int intTileType;
            double tileFullness;
            int seatId;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2 >> intTileType >> tileFullness >> seatId);
            Tile::TileType tileType = static_cast<Tile::TileType>(intTileType);
            std::vector<Tile*> selectedTiles = gameMap->rectangularRegion(x1, y1, x2, y2);
            std::vector<Tile*> affectedTiles;
            Seat* seat = nullptr;
            double claimedPercentage = 0.0;
            if(tileType == Tile::TileType::claimed)
            {
                seat = gameMap->getSeatById(seatId);
                claimedPercentage = 1.0;
            }
            for(Tile* tile : selectedTiles)
            {
                // We do not change tiles where there is something
                if((tile->numEntitiesInTile() > 0) &&
                   ((tileFullness > 0.0) || (tileType == Tile::TileType::lava) || (tileType == Tile::TileType::water)))
                    continue;
                if(tile->getCoveringBuilding() != nullptr)
                    continue;

                affectedTiles.push_back(tile);
                tile->setType(tileType);
                tile->setFullness(tileFullness);
                tile->setSeat(seat);
                tile->mClaimedPercentage = claimedPercentage;
            }
            if(!affectedTiles.empty())
            {
                int nbTiles = affectedTiles.size();
                ServerNotification notif(ServerNotification::refreshTiles, clientSocket->getPlayer());
                notif.mPacket << nbTiles;
                for(std::vector<Tile*>::iterator it = affectedTiles.begin(); it != affectedTiles.end(); ++it)
                {
                    Tile* tile = *it;
                    notif.mPacket << tile;
                }
                sendAsyncMsgToAllClients(notif);
            }
            break;
        }

        case ClientNotification::editorAskBuildRoom:
        {
            OD_ASSERT_TRUE_MSG(mServerMode == ServerMode::ModeEditor, "Received editor command while wrong mode mode"
                + Ogre::StringConverter::toString(static_cast<int>(mServerMode)));
            int x1, y1, x2, y2;
            int seatId;
            Room::RoomType type;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2 >> type >> seatId);
            Seat* seat = gameMap->getSeatById(seatId);
            OD_ASSERT_TRUE_MSG(seat != NULL, "seatId=" + Ogre::StringConverter::toString(seatId));
            std::vector<Tile*> selectedTiles = gameMap->rectangularRegion(x1, y1, x2, y2);
            std::vector<Tile*> tiles;
            // We start by changing the tiles so that the room can be built
            for(Tile* tile : selectedTiles)
            {
                // We do not change tiles where there is something on the tile
                if(tile->getCoveringBuilding() != nullptr)
                    continue;
                tiles.push_back(tile);

                tile->setType(Tile::TileType::claimed);
                tile->setSeat(seat);
                tile->setFullness(0.0);
            }

            if(tiles.empty())
                break;

            Room* room = nullptr;
            switch (type)
            {
                case Room::RoomType::nullRoomType:
                {
                    room = nullptr;
                    break;
                }
                case Room::RoomType::dormitory:
                {
                    room = new RoomDormitory(gameMap);
                    break;
                }
                case Room::RoomType::dungeonTemple:
                {
                    room = new RoomDungeonTemple(gameMap);
                    break;
                }
                case Room::RoomType::forge:
                {
                    room = new RoomForge(gameMap);
                    break;
                }
                case Room::RoomType::hatchery:
                {
                    room = new RoomHatchery(gameMap);
                    break;
                }
                case Room::RoomType::library:
                {
                    room = new RoomLibrary(gameMap);
                    break;
                }
                case Room::RoomType::portal:
                {
                    room = new RoomPortal(gameMap);
                    break;
                }
                case Room::RoomType::trainingHall:
                {
                    room = new RoomTrainingHall(gameMap);
                    break;
                }
                case Room::RoomType::treasury:
                {
                    room = new RoomTreasury(gameMap);
                    break;
                }
                case Room::RoomType::crypt:
                {
                    room = new RoomCrypt(gameMap);
                    break;
                }
                default:
                    OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                        static_cast<int>(type)));
            }
            if(room == nullptr)
                break;

            room->setupRoom(gameMap->nextUniqueNameRoom(room->getMeshName()), seat, tiles);

            gameMap->addRoom(room, true);
            room->createMesh();
            room->checkForRoomAbsorbtion();
            room->updateActiveSpots();
            gameMap->refreshBorderingTilesOf(tiles);
            break;
        }

        case ClientNotification::editorAskBuildTrap:
        {
            OD_ASSERT_TRUE_MSG(mServerMode == ServerMode::ModeEditor, "Received editor command while wrong mode mode"
                + Ogre::StringConverter::toString(static_cast<int>(mServerMode)));
            int x1, y1, x2, y2;
            int seatId;
            Trap::TrapType type;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2 >> type >> seatId);
            Seat* seat = gameMap->getSeatById(seatId);
            OD_ASSERT_TRUE_MSG(seat != NULL, "seatId=" + Ogre::StringConverter::toString(seatId));
            std::vector<Tile*> selectedTiles = gameMap->rectangularRegion(x1, y1, x2, y2);
            std::vector<Tile*> tiles;
            // We start by changing the tiles so that the trap can be built
            for(Tile* tile : selectedTiles)
            {
                // We do not change tiles where there is something
                if(tile->getCoveringBuilding() != nullptr)
                    continue;
                tiles.push_back(tile);

                tile->setType(Tile::TileType::claimed);
                tile->setSeat(seat);
                tile->setFullness(0.0);
            }

            if(tiles.empty())
                break;

            Trap* trap = nullptr;
            switch (type)
            {
                case Trap::TrapType::nullTrapType:
                {
                    trap = nullptr;
                    break;
                }
                case Trap::TrapType::cannon:
                {
                    trap = new TrapCannon(gameMap);
                    break;
                }
                case Trap::TrapType::spike:
                {
                    trap = new TrapSpike(gameMap);
                    break;
                }
                case Trap::TrapType::boulder:
                {
                    trap = new TrapBoulder(gameMap);
                    break;
                }
                default:
                    OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                        static_cast<int>(type)));
            }

            if(trap == nullptr)
                break;

            trap->setupTrap(gameMap->nextUniqueNameTrap(trap->getMeshName()), seat, tiles);
            gameMap->addTrap(trap);
            trap->createMesh();
            trap->updateActiveSpots();
            break;
        }

        case ClientNotification::editorCreateWorker:
        {
            OD_ASSERT_TRUE_MSG(mServerMode == ServerMode::ModeEditor, "Received editor command while wrong mode mode"
                + Ogre::StringConverter::toString(static_cast<int>(mServerMode)));
            Player* player = clientSocket->getPlayer();
            int seatId;
            OD_ASSERT_TRUE(packetReceived >> seatId);
            Seat* seatCreature = gameMap->getSeatById(seatId);
            OD_ASSERT_TRUE_MSG(seatCreature != NULL, "seatId=" + Ogre::StringConverter::toString(seatId));
            const CreatureDefinition *classToSpawn = gameMap->getClassDescription("Kobold");
            OD_ASSERT_TRUE(classToSpawn != nullptr);
            if(classToSpawn == nullptr)
                break;
            Creature* newCreature = new Creature(gameMap, classToSpawn);
            newCreature->setSeat(seatCreature);

            newCreature->createMesh();
            gameMap->addCreature(newCreature);

            ODPacket packetSend;
            packetSend << ServerNotification::addCreature;
            newCreature->exportToPacket(packetSend);
            sendMsgToAllClients(packetSend);

            player->pickUpEntity(newCreature, mServerMode == ServerMode::ModeEditor);
            break;
        }

        case ClientNotification::editorCreateFighter:
        {
            OD_ASSERT_TRUE_MSG(mServerMode == ServerMode::ModeEditor, "Received editor command while wrong mode mode"
                + Ogre::StringConverter::toString(static_cast<int>(mServerMode)));
            Player* player = clientSocket->getPlayer();
            int seatId;
            std::string className;
            OD_ASSERT_TRUE(packetReceived >> seatId >> className);
            Seat* seatCreature = gameMap->getSeatById(seatId);
            OD_ASSERT_TRUE_MSG(seatCreature != NULL, "seatId=" + Ogre::StringConverter::toString(seatId));
            const CreatureDefinition *classToSpawn = gameMap->getClassDescription(className);
            OD_ASSERT_TRUE_MSG(classToSpawn != nullptr, "Couldn't spawn creature class=" + className);
            if(classToSpawn == nullptr)
                break;
            Creature* newCreature = new Creature(gameMap, classToSpawn);
            newCreature->setSeat(seatCreature);

            newCreature->createMesh();
            gameMap->addCreature(newCreature);

            ODPacket packetSend;
            packetSend << ServerNotification::addCreature;
            newCreature->exportToPacket(packetSend);
            sendMsgToAllClients(packetSend);

            player->pickUpEntity(newCreature, mServerMode == ServerMode::ModeEditor);
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

bool ODServer::notifyNewConnection(ODSocketClient *clientSocket)
{
    GameMap* gameMap = mGameMap;
    if (gameMap == NULL)
        return false;

    switch(mServerState)
    {
        case ServerState::StateNone:
        {
            // It is not normal to receive new connexions while not connected. We are in an unexpected state
            OD_ASSERT_TRUE(false);
            return false;
        }
        case ServerState::StateConfiguration:
        {
            setClientState(clientSocket, "connected");
            return true;
        }
        case ServerState::StateGame:
        {
            // TODO : handle re-connexion if a client was disconnected and tries to reconnect
            return false;
        }
    }

    return false;
}

bool ODServer::notifyClientMessage(ODSocketClient *clientSocket)
{
    bool ret = processClientNotifications(clientSocket);
    if(!ret)
    {
        LogManager::getSingleton().logMessage("Client disconnected state=" + clientSocket->getState());
        if(std::string("ready").compare(clientSocket->getState()) == 0)
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
                OD_ASSERT_TRUE(false);
                exit(1);
            }
        }
        // TODO : wait at least 1 minute if the client reconnects if deconnexion happens during game
    }
    return ret;
}

void ODServer::stopServer()
{
    // We start by stopping server to make sure no new message comes
    ODSocketServer::stopServer();

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

ODPacket& operator<<(ODPacket& os, const ODServer::ServerMode& sm)
{
    os << static_cast<int32_t>(sm);
    return os;
}

ODPacket& operator>>(ODPacket& is, ODServer::ServerMode& sm)
{
    int32_t tmp;
    is >> tmp;
    sm = static_cast<ODServer::ServerMode>(tmp);
    return is;
}
