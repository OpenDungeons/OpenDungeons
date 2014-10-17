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
#include "MapLoader.h"
#include "RenderedMovableEntity.h"
#include "LogManager.h"
#include "Creature.h"
#include "Trap.h"
#include "TrapCannon.h"
#include "TrapSpike.h"
#include "TrapBoulder.h"
#include "RoomDormitory.h"
#include "RoomDungeonTemple.h"
#include "RoomForge.h"
#include "RoomHatchery.h"
#include "RoomLibrary.h"
#include "RoomPortal.h"
#include "RoomTrainingHall.h"
#include "RoomTreasury.h"

#include <SFML/Network.hpp>
#include <SFML/System.hpp>

#include "boost/filesystem.hpp"

const std::string ODServer::SERVER_INFORMATION = "SERVER_INFORMATION";

template<> ODServer* Ogre::Singleton<ODServer>::msSingleton = 0;

ODServer::ODServer() :
    mServerMode(ModeGame),
    mGameMap(new GameMap(true)),
    mNbClientsNotReady(0)
{
}

ODServer::~ODServer()
{
    delete mGameMap;
}

bool ODServer::startServer(const std::string& levelFilename, bool replaceHumanPlayersByAi, ServerMode mode)
{
    LogManager& logManager = LogManager::getSingleton();
    logManager.logMessage("Asked to launch server with levelFilename=" + levelFilename);

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
    mServerMode = mode;
    GameMap* gameMap = mGameMap;
    if (!gameMap->loadLevel(levelFilename))
    {
        logManager.logMessage("Couldn't start server. The level file can't be loaded: " + levelFilename);
        return false;
    }

    // Fill seats with either player, AIs or nothing depending on the given faction.
    uint32_t i = 0;
    uint32_t uniqueAINumber = 0;
    uint32_t uniqueInactiveNumber = 0;
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
                gameMap->addPlayer(aiPlayer, gameMap->popEmptySeat(seat->getId()));
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
            gameMap->addPlayer(aiPlayer, gameMap->popEmptySeat(seat->getId()));
            gameMap->assignAI(*aiPlayer, "KeeperAI");
            continue;
        }
        else
        {
            // We create a virtual player that does nothing
            Player* aiPlayer = new Player();
            aiPlayer->setNick("Inactive player " + Ogre::StringConverter::toString(++uniqueInactiveNumber));

            // The empty seat is removed by addPlayer(), so we loop without incrementing i
            gameMap->addPlayer(aiPlayer, gameMap->popEmptySeat(seat->getId()));
            continue;
        }

        ++i;
    }

    logManager.logMessage("Added: " + Ogre::StringConverter::toString(nbPlayerSeat) + " Human players");
    logManager.logMessage("Added: " + Ogre::StringConverter::toString(uniqueAINumber) + " AI players");
    logManager.logMessage("Added: " + Ogre::StringConverter::toString(uniqueInactiveNumber) + " inactive players");

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

    // We do not update turn in editor mode to not have creatures wande
    if(mServerMode == ServerMode::ModeGame)
    {
        gameMap->doTurn();
        gameMap->doPlayerAITurn(timeSinceLastFrame);
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
                        gameMap->addGoldToSeat(seat->mStartingGold, seat->getId());
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
                // This should not be a message as it is sent directly to the client
                OD_ASSERT_TRUE(false);
                break;

            case ServerNotification::entityDropped:
                // This should not be a message as it is sent directly to the client
                OD_ASSERT_TRUE(false);
                break;

            case ServerNotification::buildRoom:
            {
                // This message should only be sent by ai players (human players are notified directly)
                std::string& faction = event->mConcernedPlayer->getSeat()->mFaction;
                OD_ASSERT_TRUE_MSG(faction != "Player", faction);
                sendMsgToAllClients(event->mPacket);
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
                int seatId = seat->getId();
                clientSocket->setPlayer(curPlayer);
                gameMap->addPlayer(curPlayer, gameMap->popEmptySeat(seatId));
                // We notify the newly connected player to the others
                packetSend.clear();
                packetSend << ServerNotification::addPlayer << clientNick
                    << seatId;
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
            int seatId = seat->getId();
            packetSend << ServerNotification::yourSeat << seatId;
            sendMsgToClient(clientSocket, packetSend);

            // We notify the pending players to the new one
            for (unsigned int i = 0; i < gameMap->numPlayers(); ++i)
            {
                // Don't tell the client about its own player structure
                Player* tempPlayer = gameMap->getPlayer(i);
                if (curPlayer != tempPlayer && tempPlayer != NULL)
                {
                    packetSend.clear();
                    int seatId = tempPlayer->getSeat()->getId();
                    std::string nick = tempPlayer->getNick();
                    packetSend << ServerNotification::addPlayer << nick << seatId;
                    sendMsgToClient(clientSocket, packetSend);
                }
            }

            // The player received everything. He is ready
            setClientState(clientSocket, "ready");
            // We notify the client that everything is ready
            packetSend.clear();
            int32_t intServerMode = static_cast<int32_t>(mServerMode);
            packetSend << ServerNotification::clientAccepted << intServerMode;
            sendMsgToClient(clientSocket, packetSend);
            --mNbClientsNotReady;
            LogManager::getSingleton().logMessage("Player=" + curPlayer->getNick()
                + " has been accepted in the game on seatId="
                + Ogre::StringConverter::toString(seat->getId()));
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

            GameEntity* entity = nullptr;
            Player *player = clientSocket->getPlayer();
            bool allowPickup = false;
            switch(entityType)
            {
                case GameEntity::ObjectType::creature:
                {
                    Creature* creature = gameMap->getCreature(entityName);
                    entity = creature;
                    allowPickup = creature->tryPickup(player->getSeat(), mServerMode == ServerMode::ModeEditor);
                    break;
                }
                case GameEntity::ObjectType::renderedMovableEntity:
                {
                    RenderedMovableEntity* obj = gameMap->getRenderedMovableEntity(entityName);
                    entity = obj;
                    allowPickup = obj->tryPickup(player->getSeat(), mServerMode == ServerMode::ModeEditor);
                    break;
                }
                default:
                    // No need to display an error as it will be displayed in the following assert
                    break;
            }
            OD_ASSERT_TRUE_MSG(entity != nullptr, "entityType=" + Ogre::StringConverter::toString(static_cast<int32_t>(entityType)) + ", entityName=" + entityName);
            if(entity == nullptr)
                break;

            if(!allowPickup)
            {
                LogManager::getSingleton().logMessage("player=" + player->getNick()
                        + " could not pickup entity entityType="
                        + Ogre::StringConverter::toString(static_cast<int32_t>(entityType))
                        + ", entityName=" + entityName);
                break;
            }

            int seatId = player->getSeat()->getId();
            player->pickUpEntity(entity, mServerMode == ServerMode::ModeEditor);
            // We notify the players
            // We notify the other players that a creature has been picked up
            ODPacket packetSend;
            packetSend << ServerNotification::entityPickedUp;
            packetSend << mServerMode << seatId << entityType << entityName;
            sendMsgToAllClients(packetSend);
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
                    int seatId = player->getSeat()->getId();
                    ODPacket packet;
                    packet << ServerNotification::entityDropped;
                    packet << seatId << tile;
                    sendMsgToAllClients(packet);
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
                default:
                    OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                        static_cast<int>(type)));
            }
            if(room == nullptr)
                break;

            room->setupRoom(gameMap->nextUniqueNameRoom(room->getMeshName()), player->getSeat(), tiles);
            gameMap->addRoom(room);
            room->checkForRoomAbsorbtion();
            room->createMesh();
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
            for(std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end(); ++it)
            {
                Tile* tile = *it;
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
            int x1, y1, x2, y2;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2);
            std::vector<Tile*> tiles = gameMap->rectangularRegion(x1, y1, x2, y2);

            if(tiles.empty())
                break;

            std::set<Room*> roomsImpacted;
            std::vector<Tile*> sentTiles;
            for(std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end(); ++it)
            {
                Tile* tile = *it;
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
            for(std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end(); ++it)
            {
                Tile* tile = *it;
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
            int x1, y1, x2, y2;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2);
            std::vector<Tile*> tiles = gameMap->rectangularRegion(x1, y1, x2, y2);

            if(tiles.empty())
                break;

            std::set<Trap*> trapsImpacted;
            std::vector<Tile*> sentTiles;
            for(std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end(); ++it)
            {
                Tile* tile = *it;
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
            int x1, y1, x2, y2;
            int intTileType;
            double tileFullness;
            int seatId;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2 >> intTileType >> tileFullness >> seatId);
            Tile::TileType tileType = static_cast<Tile::TileType>(intTileType);
            std::vector<Tile*> selectedTiles = gameMap->rectangularRegion(x1, y1, x2, y2);
            std::vector<Tile*> affectedTiles;
            Seat* seat = NULL;
            double claimedPercentage = 0.0;
            if(tileType == Tile::TileType::claimed)
            {
                seat = gameMap->getSeatById(seatId);
                claimedPercentage = 1.0;
            }
            for(std::vector<Tile*>::iterator it = selectedTiles.begin(); it != selectedTiles.end(); ++it)
            {
                Tile* tile = *it;
                if(tile == NULL)
                    continue;

                // We do not change tiles where there is something
                if((tile->numCreaturesInCell() > 0) &&
                   ((tileFullness > 0.0) || (tileType == Tile::TileType::lava) || (tileType == Tile::TileType::water)))
                    continue;
                if(tile->getCoveringRoom() != NULL)
                    continue;
                if(tile->getCoveringTrap() != NULL)
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
            int x1, y1, x2, y2;
            int seatId;
            Room::RoomType type;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2 >> type >> seatId);
            Player* player = gameMap->getPlayerBySeatId(seatId);
            OD_ASSERT_TRUE_MSG(player != NULL, "seatId=" + Ogre::StringConverter::toString(seatId));
            std::vector<Tile*> selectedTiles = gameMap->rectangularRegion(x1, y1, x2, y2);
            std::vector<Tile*> tiles;
            // We start by changing the tiles so that the room can be built
            for(std::vector<Tile*>::iterator it = selectedTiles.begin(); it != selectedTiles.end(); ++it)
            {
                Tile* tile = *it;
                if(tile == NULL)
                    continue;

                // We do not change tiles where there is something on the tile
                if(tile->getCoveringRoom() != NULL)
                    continue;
                if(tile->getCoveringTrap() != NULL)
                    continue;
                tiles.push_back(tile);

                tile->setType(Tile::TileType::claimed);
                tile->setSeat(player->getSeat());
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
                default:
                    OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                        static_cast<int>(type)));
            }
            if(room == nullptr)
                break;

            room->setupRoom(gameMap->nextUniqueNameRoom(room->getMeshName()), player->getSeat(), tiles);

            gameMap->addRoom(room);
            room->checkForRoomAbsorbtion();
            room->createMesh();
            room->updateActiveSpots();
            gameMap->refreshBorderingTilesOf(tiles);
            break;
        }

        case ClientNotification::editorAskBuildTrap:
        {
            int x1, y1, x2, y2;
            int seatId;
            Trap::TrapType type;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2 >> type >> seatId);
            Player* player = gameMap->getPlayerBySeatId(seatId);
            OD_ASSERT_TRUE_MSG(player != NULL, "seatId=" + Ogre::StringConverter::toString(seatId));
            std::vector<Tile*> selectedTiles = gameMap->rectangularRegion(x1, y1, x2, y2);
            std::vector<Tile*> tiles;
            // We start by changing the tiles so that the trap can be built
            for(std::vector<Tile*>::iterator it = selectedTiles.begin(); it != selectedTiles.end(); ++it)
            {
                Tile* tile = *it;
                if(tile == NULL)
                    continue;

                // We do not change tiles where there is something
                if(tile->getCoveringRoom() != NULL)
                    continue;
                if(tile->getCoveringTrap() != NULL)
                    continue;
                tiles.push_back(tile);

                tile->setType(Tile::TileType::claimed);
                tile->setSeat(player->getSeat());
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

            trap->setupTrap(gameMap->nextUniqueNameTrap(trap->getMeshName()), player->getSeat(), tiles);
            gameMap->addTrap(trap);
            trap->createMesh();
            trap->updateActiveSpots();
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
            OD_ASSERT_TRUE(false);
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
