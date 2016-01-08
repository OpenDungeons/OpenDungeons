/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
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

#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/GameEntityType.h"
#include "entities/MapLight.h"
#include "entities/Tile.h"
#include "entities/Weapon.h"
#include "game/Player.h"
#include "game/Skill.h"
#include "game/SkillManager.h"
#include "game/SkillType.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "gamemap/MapHandler.h"
#include "modes/ConsoleCommands.h"
#include "network/ODClient.h"
#include "network/ServerMode.h"
#include "network/ServerNotification.h"
#include "rooms/RoomManager.h"
#include "rooms/RoomType.h"
#include "spells/SpellManager.h"
#include "spells/SpellType.h"
#include "traps/Trap.h"
#include "traps/TrapType.h"
#include "traps/TrapManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MasterServer.h"
#include "utils/ResourceManager.h"
#include "ODApplication.h"

#include <SFML/Network.hpp>
#include <SFML/System.hpp>

#include <boost/algorithm/string/join.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>


const std::string SAVEGAME_SKIRMISH_PREFIX = "SK-";
const std::string SAVEGAME_MULTIPLAYER_PREFIX = "MP-";
static const double MASTER_SERVER_UPDATE_PERIOD_MS = 30000.0;
static const int32_t MASTER_SERVER_STATUS_PENDING = 0;
static const int32_t MASTER_SERVER_STATUS_STARTED = 1;
static const int32_t MASTER_SERVER_STATUS_FINISHED = 2;

template<> ODServer* Ogre::Singleton<ODServer>::msSingleton = nullptr;

ODServer::ODServer() :
    mUniqueNumberPlayer(0),
    mServerMode(ServerMode::ModeNone),
    mServerState(ServerState::StateNone),
    mGameMap(new GameMap(true)),
    mSeatsConfigured(false),
    mPlayerConfig(nullptr),
    mConsoleInterface(std::bind(&ODServer::printConsoleMsg, this, std::placeholders::_1)),
    mMasterServerGameStatusUpdateTime(0)
{
    ConsoleCommands::addConsoleCommands(mConsoleInterface);
}

ODServer::~ODServer()
{
    delete mGameMap;
}

bool ODServer::startServer(const std::string& creator, const std::string& levelFilename, ServerMode mode, bool useMasterServer)
{
    OD_LOG_INF("Asked to launch server with levelFilename=" + levelFilename);

    mSeatsConfigured = false;
    mDisconnectedPlayers.clear();
    mMasterServerGameId.clear();
    mMasterServerGameStatusUpdateTime = 0.0;
    mPlayerConfig = nullptr;

    // Start the server socket listener as well as the server socket thread
    if (isConnected())
    {
        OD_LOG_INF("Couldn't start server: The server is already connected");
        return false;
    }
    if ((ODClient::getSingletonPtr() != nullptr) &&
        ODClient::getSingleton().isConnected())
    {
        OD_LOG_INF("Couldn't start server: The client is already connected");
        return false;
    }

    // Set up the socket to listen on the specified port
    int32_t port = getNetworkPort();
    if (!createServer(port))
    {
        mServerMode = ServerMode::ModeNone;
        mServerState = ServerState::StateNone;
        OD_LOG_ERR("Server could not create server socket!");
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
        OD_LOG_INF("Couldn't start server. The level file can't be loaded: " + levelFilename);
        return false;
    }
    mUniqueNumberPlayer = 0;

    // We configure what is fixed (fixed AI, faction or team). While iterating seats, we keep in mind if there is
    // at least a human only seat. If yes, we configure all player type choosable to AI. If not, we configure all player
    // type choosable to AI except the first one.
    uint32_t nbSeatsHuman = 0;
    const std::vector<std::string>& factions = ConfigManager::getSingleton().getFactions();
    for(Seat* seat : gameMap->getSeats())
    {
        if(seat->isRogueSeat())
            continue;

        // Player faction
        if(seat->getFaction().compare(Seat::PLAYER_FACTION_CHOICE) != 0)
        {
            uint32_t cptFaction = 0;
            for(const std::string& faction : factions)
            {
                if(seat->getFaction().compare(faction) == 0)
                    break;

                ++cptFaction;
            }
            // If the faction is not found, we set it to the first defined
            if(cptFaction >= factions.size())
                cptFaction = 0;

            seat->setConfigFactionIndex(cptFaction);
        }

        // Player type
        if(seat->getPlayerType().compare(Seat::PLAYER_TYPE_INACTIVE) == 0)
            seat->setConfigPlayerId(0);
        else if(seat->getPlayerType().compare(Seat::PLAYER_TYPE_AI) == 0)
            seat->setConfigPlayerId(1);
        else if(seat->getPlayerType().compare(Seat::PLAYER_TYPE_HUMAN) == 0)
            ++nbSeatsHuman;

        // Player team
        const std::vector<int>& availableTeamIds = seat->getAvailableTeamIds();
        if(availableTeamIds.size() == 1)
        {
            seat->setConfigTeamId(availableTeamIds.front());
        }
    }

    if(useMasterServer)
    {
        LevelInfo info;
        if(!MapHandler::getMapInfo(levelFilename, info))
        {
            info.mLevelName = "No name";
            info.mLevelDescription = "No description";
        }

        const std::string& label = info.mLevelName;
        const std::string& descr = info.mLevelDescription;
        std::string uuid;
        if(!MasterServer::registerGame(ODApplication::VERSION, creator, port, label, descr, uuid))
        {
            OD_LOG_ERR("Could not register the game in the master server !!!");
            return false;
        }

        mMasterServerGameId = uuid;
    }

    // In single player, we use a default value for seats that can be chosen
    if(mServerMode != ServerMode::ModeGameSinglePlayer)
        return true;

    for(Seat* seat : gameMap->getSeats())
    {
        if(seat->isRogueSeat())
            continue;

        // Player faction to the first one defined
        if(seat->getFaction().compare(Seat::PLAYER_FACTION_CHOICE) == 0)
            seat->setConfigFactionIndex(0);

        // Player type to AI
        if(seat->getPlayerType().compare(Seat::PLAYER_TYPE_CHOICE) == 0)
        {
            if(nbSeatsHuman == 0)
                ++nbSeatsHuman;
            else
                seat->setConfigPlayerId(1);
        }

        // Player team to first one available
        const std::vector<int>& availableTeamIds = seat->getAvailableTeamIds();
        if(availableTeamIds.size() > 1)
        {
            seat->setConfigTeamId(availableTeamIds.front());
        }
    }

    return true;
}

void ODServer::queueServerNotification(ServerNotification* n)
{
    if ((n == nullptr) || (!isConnected()))
    {
        delete n;
        return;
    }
    mServerNotificationQueue.push_back(n);
}

void ODServer::sendAsyncMsg(ServerNotification& notif)
{
    sendMsg(notif.mConcernedPlayer, notif.mPacket);
}

void ODServer::sendMsg(Player* player, ODPacket& packet)
{
    if(player == nullptr)
    {
        // If player is nullptr, we send the message to every connected player
        for (ODSocketClient* client : mSockClients)
            client->send(packet);

        return;
    }

    ODSocketClient* client = getClientFromPlayer(player);
    if((client == nullptr) &&
       (std::find(mDisconnectedPlayers.begin(), mDisconnectedPlayers.end(), player) == mDisconnectedPlayers.end()))
    {
        ServerNotificationType type;
        OD_ASSERT_TRUE(packet >> type);
        OD_ASSERT_TRUE_MSG(client != nullptr, "player=" + player->getNick()
            + ", ServerNotificationType=" + ServerNotification::typeString(type));
        return;
    }

    if(client != nullptr)
        client->send(packet);
}

void ODServer::handleConsoleCommand(Player* player, GameMap* gameMap, const std::vector<std::string>& args)
{
    if(args.empty())
    {
        OD_LOG_WRN("Invalid empty console command");
        return;
    }

    if(mConsoleInterface.tryExecuteServerCommand(args, *gameMap) != Command::Result::SUCCESS)
    {
        std::string msg = "Cannot execute console command";
        for(const std::string& str : args)
        {
            msg += " " + str;
        }
        OD_LOG_WRN(msg);
        return;
    }

    // We notify all players that a console command has been executed
    ServerNotification *serverNotification = new ServerNotification(
        ServerNotificationType::chatServer, nullptr);

    std::string msg = "Console cmd launched: " + args[0];
    serverNotification->mPacket << msg << EventShortNoticeType::genericGameInfo;
    ODServer::getSingleton().queueServerNotification(serverNotification);
}

void ODServer::startNewTurn(double timeSinceLastTurn)
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

    ServerNotification* serverNotification = new ServerNotification(
        ServerNotificationType::turnStarted, nullptr);
    serverNotification->mPacket << turn;
    queueServerNotification(serverNotification);

    if(mServerMode == ServerMode::ModeEditor)
        gameMap->updateVisibleEntities();

    gameMap->updateAnimations(timeSinceLastTurn);

    // We notify the clients about what they got
    for (ODSocketClient* sock : mSockClients)
    {
        Player* player = sock->getPlayer();
        // For now, only the player whose seat changed is notified. If we need it, we could send the event to every player
        // so that they can see how far from the goals the other players are
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::refreshPlayerSeat, player);
        std::string goals = gameMap->getGoalsStringForPlayer(player);
        Seat* seat = player->getSeat();
        seat->exportToPacketForUpdate(serverNotification->mPacket);
        serverNotification->mPacket << goals;
        ODServer::getSingleton().queueServerNotification(serverNotification);

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

                ServerNotification *serverNotification = new ServerNotification(
                    ServerNotificationType::notifyCreatureInfo, player);
                serverNotification->mPacket << name << creatureInfos;
                ODServer::getSingleton().queueServerNotification(serverNotification);

                ++itCreatures;
            }
        }
    }

    switch(mServerMode)
    {
        case ServerMode::ModeGameSinglePlayer:
        case ServerMode::ModeGameMultiPlayer:
        case ServerMode::ModeGameLoaded:
        {
            gameMap->doTurn(timeSinceLastTurn);
            gameMap->doPlayerAITurn(timeSinceLastTurn);
            break;
        }
        case ServerMode::ModeEditor:
            // We do not update turn in editor mode to not have creatures wander
            break;
        case ServerMode::ModeNone:
            // It is not normal to have no mode selected and starting turns
            OD_LOG_ERR("Wrong none server mode");
            break;
        default:
            break;
    }

    gameMap->updateVisibleEntities();
    gameMap->processActiveObjectsChanges();
    gameMap->processDeletionQueues();
}

void ODServer::serverThread()
{
    GameMap* gameMap = mGameMap;
    sf::Clock clock;
    double turnLengthMs = 1000.0 / ODApplication::turnsPerSecond;
    bool isClientConnected = true;
    while(isConnected() && isClientConnected)
    {
        // doTask should return after the length of 1 turn even if their are communications. When
        // it returns, we can launch next turn.
        doTask(static_cast<int32_t>(turnLengthMs));
        // If all the clients are disconnected during a game, we close the server
        if((mServerState == ServerState::StateGame) &&
           (mSockClients.empty()))
        {
            // Time to stop the game
            isClientConnected = false;
            continue;
        }

        if(gameMap->getTurnNumber() == -1)
        {
            // The game is not started
            if(mSeatsConfigured)
            {
                // We notify the master server that we are not waiting for players anymore
                if(!mMasterServerGameId.empty())
                {
                    mMasterServerGameStatusUpdateTime = 0.0;
                    MasterServer::updateGame(mMasterServerGameId, MASTER_SERVER_STATUS_STARTED);
                }

                // We configure the game for launching
                const std::vector<Seat*>& seats = gameMap->getSeats();
                for (int jj = 0; jj < gameMap->getMapSizeY(); ++jj)
                {
                    for (int ii = 0; ii < gameMap->getMapSizeX(); ++ii)
                    {
                        Tile* tile = gameMap->getTile(ii,jj);
                        tile->setSeats(seats);
                    }
                }

                // We set allied seats
                for(Seat* seat : seats)
                {
                    for(Seat* alliedSeat : seats)
                    {
                        if(alliedSeat == seat)
                            continue;
                        if(!seat->isAlliedSeat(alliedSeat))
                            continue;
                        seat->addAlliedSeat(alliedSeat);
                    }
                }

                // Every client is connected and ready, we can launch the game
                // Send turn 0 to init the map
                ServerNotification* serverNotification = new ServerNotification(
                    ServerNotificationType::turnStarted, nullptr);
                serverNotification->mPacket << static_cast<int64_t>(0);
                queueServerNotification(serverNotification);

                OD_LOG_INF("Server ready, starting game");
                gameMap->setTurnNumber(0);
                gameMap->setGamePaused(false);

                // In editor mode, we give vision on all the gamemap tiles
                if(mServerMode == ServerMode::ModeEditor)
                {
                    for (Seat* seat : gameMap->getSeats())
                    {
                        for (int jj = 0; jj < gameMap->getMapSizeY(); ++jj)
                        {
                            for (int ii = 0; ii < gameMap->getMapSizeX(); ++ii)
                            {
                                gameMap->getTile(ii,jj)->notifyVision(seat);
                            }
                        }

                        seat->sendVisibleTiles();
                    }
                }

                gameMap->createAllEntities();

                // Fill starting gold
                for(Seat* seat : gameMap->getSeats())
                {
                    if(seat->getPlayer() == nullptr)
                        continue;

                    if(seat->getGold() > 0)
                        gameMap->addGoldToSeat(seat->getGold(), seat->getId());
                }
            }
            else
            {
                // We are still waiting for players
                if(!mMasterServerGameId.empty())
                {
                    mMasterServerGameStatusUpdateTime += turnLengthMs;
                    if(mMasterServerGameStatusUpdateTime >= MASTER_SERVER_UPDATE_PERIOD_MS)
                    {
                        mMasterServerGameStatusUpdateTime = 0.0;
                        MasterServer::updateGame(mMasterServerGameId, MASTER_SERVER_STATUS_PENDING);
                    }
                }
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
    }

    if(!mMasterServerGameId.empty())
    {
        mMasterServerGameStatusUpdateTime = 0.0;
        MasterServer::updateGame(mMasterServerGameId, MASTER_SERVER_STATUS_FINISHED);
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

        if(event == nullptr)
        {
            OD_LOG_ERR("unexpected null event");
            continue;
        }

        switch (event->mType)
        {
            case ServerNotificationType::turnStarted:
                OD_LOG_INF("Server sends newturn="
                    + boost::lexical_cast<std::string>(gameMap->getTurnNumber()));
                sendMsg(event->mConcernedPlayer, event->mPacket);
                break;

            case ServerNotificationType::entityPickedUp:
                // This message should not be sent by human players (they are notified asynchronously)
                OD_ASSERT_TRUE_MSG(event->mConcernedPlayer->getIsHuman(), "nick=" + event->mConcernedPlayer->getNick());
                sendMsg(event->mConcernedPlayer, event->mPacket);
                break;

            case ServerNotificationType::entityDropped:
                // This message should not be sent by human players (they are notified asynchronously)
                OD_ASSERT_TRUE_MSG(event->mConcernedPlayer->getIsHuman(), "nick=" + event->mConcernedPlayer->getNick());
                sendMsg(event->mConcernedPlayer, event->mPacket);
                break;

            case ServerNotificationType::entitySlapped:
                // This message should not be sent by human players (they are notified asynchronously)
                OD_ASSERT_TRUE_MSG(!event->mConcernedPlayer->getIsHuman(), "nick=" + event->mConcernedPlayer->getNick());
                sendMsg(event->mConcernedPlayer, event->mPacket);
                break;

            case ServerNotificationType::exit:
                running = false;
                stopServer();
                break;

            default:
                sendMsg(event->mConcernedPlayer, event->mPacket);
                break;
        }

        delete event;
        event = nullptr;
    }
}

bool ODServer::processClientNotifications(ODSocketClient* clientSocket)
{
    if (!clientSocket)
        return false;

    GameMap* gameMap = mGameMap;

    ODPacket packetReceived;

    ODSocketClient::ODComStatus status = clientSocket->recv(packetReceived);

    // If the client closed the connection
    if (status != ODSocketClient::ODComStatus::OK)
    {
        // If a client disconnects during seat configuration, we delete him from the list
        if(mServerState != ServerState::StateConfiguration)
            return (status != ODSocketClient::ODComStatus::Error);

        // If the client is in a state where he has been notified to the other clients,
        // we notify his deconnexion
        if(std::string("ready").compare(clientSocket->getState()) != 0)
            return (status != ODSocketClient::ODComStatus::Error);

        OD_LOG_INF("Disconnected player: " + clientSocket->getPlayer()->getNick());
        // We notify
        uint32_t nbPlayers = 1;
        ODPacket packetSend;
        packetSend << ServerNotificationType::removePlayers << nbPlayers;
        int32_t id = clientSocket->getPlayer()->getId();
        packetSend << id;
        sendMsg(nullptr, packetSend);

        ODSocketClient* otherHumanConnected = nullptr;
        for(Seat* seat : gameMap->getSeats())
        {
            if(seat->getConfigPlayerId() == id)
            {
                seat->setConfigPlayerId(-1);
                if(clientSocket->getPlayer() == mPlayerConfig)
                    mPlayerConfig = nullptr;
            }

            if(seat->getConfigPlayerId() < 10)
                continue;

            otherHumanConnected = getClientFromPlayerId(seat->getConfigPlayerId());
        }
        if((mPlayerConfig == nullptr) &&
           (otherHumanConnected != nullptr))
        {
            mPlayerConfig = otherHumanConnected->getPlayer();
            ODPacket packetSend;
            packetSend << ServerNotificationType::playerConfigChange;
            otherHumanConnected->send(packetSend);

            OD_LOG_INF("Changing game host to " + mPlayerConfig->getNick());
        }
        return (status != ODSocketClient::ODComStatus::Error);
    }

    ClientNotificationType clientCommand;
    OD_ASSERT_TRUE(packetReceived >> clientCommand);

    switch(clientCommand)
    {
        case ClientNotificationType::hello:
        {
            if(std::string("connected").compare(clientSocket->getState()) != 0)
                return false;
            std::string version;
            OD_ASSERT_TRUE(packetReceived >> version);

            // If the version is different, we refuse the client
            if(version.compare(std::string("OpenDungeons V ") + ODApplication::VERSION) != 0)
            {
                OD_LOG_INF("Server rejected client. Application version mismatch: required= "
                    + ODApplication::VERSION + ", received=" + version);
                return false;
            }

            // Tell the client to load the given map
            OD_LOG_INF("Level sent to client: " + gameMap->getLevelName());
            clientSocket->setState("loadLevel");
            int32_t mapSizeX = gameMap->getMapSizeX();
            int32_t mapSizeY = gameMap->getMapSizeY();

            ODPacket packet;
            packet << ServerNotificationType::loadLevel;
            packet << version;
            packet << mapSizeX << mapSizeY;
            // Map infos
            packet << gameMap->getLevelName();
            packet << gameMap->getLevelDescription();
            packet << gameMap->getLevelMusicFile();
            packet << gameMap->getLevelFightMusicFile();

            packet << gameMap->getTileSetName();

            int32_t nb;
            // Seats
            const std::vector<Seat*>& seats = gameMap->getSeats();
            nb = seats.size();
            packet << nb;
            for(Seat* seat : seats)
                seat->exportToPacket(packet);

            // Creature definitions
            nb = gameMap->numClassDescriptions();
            packet << nb;
            for(int32_t i = 0; i < nb; ++i)
            {
                const CreatureDefinition* def = gameMap->getClassDescription(i);
                packet << def;
            }

            // Weapons
            nb = gameMap->numWeapons();
            packet << nb;
            for(int32_t i = 0; i < nb; ++i)
            {
                const Weapon* def = gameMap->getWeapon(i);
                packet << def;
            }

            // Tiles
            std::vector<Tile*> goldTiles;
            std::vector<Tile*> rockTiles;
            std::vector<Tile*> gemTiles;
            for (int xxx = 0; xxx < mapSizeX; ++xxx)
            {
                for (int yyy = 0; yyy < mapSizeY; ++yyy)
                {
                    Tile* tile = gameMap->getTile(xxx,yyy);
                    switch(tile->getType())
                    {
                        case TileType::gold:
                            goldTiles.push_back(tile);
                            break;
                        case TileType::rock:
                            rockTiles.push_back(tile);
                            break;
                        case TileType::gem:
                            gemTiles.push_back(tile);
                            break;
                        default:
                            // Per default, tiles are dirt and don't need to be notified
                            break;
                    }
                }
            }

            nb = goldTiles.size();
            packet << nb;
            for(Tile* tile : goldTiles)
            {
                gameMap->tileToPacket(packet, tile);
            }

            nb = rockTiles.size();
            packet << nb;
            for(Tile* tile : rockTiles)
            {
                gameMap->tileToPacket(packet, tile);
            }

            nb = gemTiles.size();
            packet << nb;
            for(Tile* tile : gemTiles)
            {
                gameMap->tileToPacket(packet, tile);
            }

            clientSocket->send(packet);
            break;
        }

        case ClientNotificationType::levelOK:
        {
            if(std::string("loadLevel").compare(clientSocket->getState()) != 0)
                return false;

            clientSocket->setState("nick");
            // Tell the client to give us their nickname
            ODPacket packetSend;
            packetSend << ServerNotificationType::pickNick << mServerMode;
            clientSocket->send(packetSend);
            break;
        }

        case ClientNotificationType::setNick:
        {
            if(std::string("nick").compare(clientSocket->getState()) != 0)
                return false;

            // Pick nick
            std::string clientNick;
            OD_ASSERT_TRUE(packetReceived >> clientNick);

            // NOTE : playerId 0 is reserved for inactive players and 1 is reserved for AI
            int32_t playerId = mUniqueNumberPlayer + 10;
            mUniqueNumberPlayer++;
            Player* curPlayer = new Player(gameMap, playerId);
            curPlayer->setNick(clientNick);
            curPlayer->setIsHuman(true);
            clientSocket->setPlayer(curPlayer);
            clientSocket->setState("ready");

            OD_LOG_INF("Player id: " + Helper::toString(playerId) + " nickname is: " + clientNick);

            if(mServerMode != ServerMode::ModeEditor)
                break;

            // On editor mode, we configure automatically seats
            mServerState = ServerState::StateGame;
            const std::vector<Seat*>& seats = gameMap->getSeats();
            if(seats.empty())
            {
                OD_LOG_ERR("unexpected empty seats in gamemap");
                break;
            }

            // By default, the first player to connect is the one allowed to configure game
            if(mPlayerConfig == nullptr)
            {
                mPlayerConfig = curPlayer;
                ODPacket packetSend;
                packetSend << ServerNotificationType::playerConfigChange;
                clientSocket->send(packetSend);
            }

            Seat* seat = seats[0];
            seat->setPlayer(curPlayer);
            //This makes sure the player is deleted on exit.
            gameMap->addPlayer(curPlayer);
            ODPacket packetSend;
            packetSend << ServerNotificationType::clientAccepted << ODApplication::turnsPerSecond;
            int32_t nbPlayers = 1;
            packetSend << nbPlayers;
            const std::string& nick = clientSocket->getPlayer()->getNick();
            int32_t id = clientSocket->getPlayer()->getId();
            int32_t seatId = seat->getId();
            int32_t teamId = 0;
            seat->setMapSize(gameMap->getMapSizeX(), gameMap->getMapSizeY());
            packetSend << nick << id << seatId << teamId;
            clientSocket->send(packetSend);

            packetSend.clear();
            packetSend << ServerNotificationType::startGameMode << seatId << mServerMode;
            clientSocket->send(packetSend);
            mSeatsConfigured = true;
            break;
        }

        case ClientNotificationType::readyForSeatConfiguration:
        {
            if(std::string("ready").compare(clientSocket->getState()) != 0)
                return false;

            // By default, the first player to connect is the one allowed to configure game
            if(mPlayerConfig == nullptr)
            {
                mPlayerConfig = clientSocket->getPlayer();
                OD_LOG_INF("New player host: " + mPlayerConfig->getNick());
                ODPacket packetSend;
                packetSend << ServerNotificationType::playerConfigChange;
                clientSocket->send(packetSend);
            }

            ODPacket packetSend;
            OD_LOG_INF("New player: " + clientSocket->getPlayer()->getNick());
            // We notify to the newly connected player all the currently connected players (including himself)
            uint32_t nbPlayers = mSockClients.size();
            packetSend << ServerNotificationType::addPlayers << nbPlayers;
            for (ODSocketClient* client : mSockClients)
            {
                const std::string& nick = client->getPlayer()->getNick();
                int32_t id = client->getPlayer()->getId();
                packetSend << nick << id;
            }
            clientSocket->send(packetSend);

            // Then, we notify the newly connected client to every client
            const std::string& clientNick = clientSocket->getPlayer()->getNick();
            int32_t clientPlayerId = clientSocket->getPlayer()->getId();
            packetSend.clear();
            nbPlayers = 1;
            packetSend << ServerNotificationType::addPlayers << nbPlayers;
            packetSend << clientNick << clientPlayerId;
            for (ODSocketClient* client : mSockClients)
            {
                if(clientSocket == client)
                    continue;

                client->send(packetSend);
            }

            // Then we look for the first available human seat and assign the player there (if available)
            Seat* seatToUse = nullptr;
            // If we find an unused human only seat, we use it. If not, we take the first choosable
            for(Seat* seat : gameMap->getSeats())
            {
                if(seat->isRogueSeat())
                    continue;
                if((seat->getPlayerType().compare(Seat::PLAYER_TYPE_HUMAN) != 0) &&
                   (seat->getPlayerType().compare(Seat::PLAYER_TYPE_CHOICE) != 0))
                {
                    continue;
                }

                if(seat->getConfigPlayerId() != -1)
                    continue;

                //Suitable seat
                if(seat->getPlayerType().compare(Seat::PLAYER_TYPE_HUMAN) == 0)
                {
                    seatToUse = seat;
                    break;
                }

                if(seatToUse == nullptr)
                    seatToUse = seat;
            }

            if(seatToUse != nullptr)
            {
                OD_LOG_INF("Player: " + clientSocket->getPlayer()->getNick() + " on seat " + Helper::toString(seatToUse->getId()));
                seatToUse->setConfigPlayerId(clientSocket->getPlayer()->getId());
                fireSeatConfigurationRefresh();
            }

            break;
        }

        case ClientNotificationType::seatConfigurationRefresh:
        {
            const std::vector<std::string>& factions = ConfigManager::getSingleton().getFactions();
            for(Seat* seat : gameMap->getSeats())
            {
                // Rogue seat do not have to be configured
                if(seat->isRogueSeat())
                    continue;

                int seatId;
                bool isSet;
                OD_ASSERT_TRUE(packetReceived >> seatId);
                OD_ASSERT_TRUE_MSG(seatId == seat->getId(), "seatId=" + Helper::toString(seatId) + ", seat->getId()=" + Helper::toString(seat->getId()));

                OD_ASSERT_TRUE(packetReceived >> isSet);
                int32_t factionIndex = -1;
                if(isSet)
                {
                    OD_ASSERT_TRUE(packetReceived >> factionIndex);
                    if(static_cast<uint32_t>(factionIndex) >= factions.size())
                        factionIndex = -1;
                }
                seat->setConfigFactionIndex(factionIndex);

                OD_ASSERT_TRUE(packetReceived >> isSet);
                int32_t playerId = -1;
                if(isSet)
                {
                    OD_ASSERT_TRUE(packetReceived >> playerId);
                }
                seat->setConfigPlayerId(playerId);

                OD_ASSERT_TRUE(packetReceived >> isSet);
                int32_t teamId = -1;
                if(isSet)
                {
                    OD_ASSERT_TRUE(packetReceived >> teamId);
                }
                seat->setConfigTeamId(teamId);
            }
            fireSeatConfigurationRefresh();
            break;
        }

        case ClientNotificationType::seatConfigurationSet:
        {
            // We change server state to make sure no new client will be accepted
            if(mServerState != ServerState::StateConfiguration)
            {
                OD_LOG_ERR("Wrong server state=" + Helper::toString(static_cast<int>(mServerState)));
                break;
            }

            bool isConfigured = true;
            for(Seat* seat : gameMap->getSeats())
            {
                // Rogue seat do not have to be configured
                if(seat->isRogueSeat())
                    continue;

                int seatId = seat->getId();
                if(seat->getConfigPlayerId() == -1)
                {
                    OD_LOG_ERR("player not configured seatId=" + Helper::toString(seatId) + ", ConfigPlayerId=" + Helper::toString(seat->getConfigPlayerId()));
                    isConfigured = false;
                    break;
                }
                if(seat->getConfigTeamId() == -1)
                {
                    OD_LOG_ERR("player not configured seatId=" + Helper::toString(seatId) + ", ConfigTeamId=" + Helper::toString(seat->getConfigTeamId()));
                    isConfigured = false;
                    break;
                }
                if(seat->getConfigFactionIndex() == -1)
                {
                    OD_LOG_ERR("player not configured seatId=" + Helper::toString(seatId) + ", ConfigFactionIndex=" + Helper::toString(seat->getConfigFactionIndex()));
                    isConfigured = false;
                    break;
                }
            }

            // If configuration is not complete, we don't go further
            if(!isConfigured)
                break;

            mServerState = ServerState::StateGame;

            const std::vector<std::string>& factions = ConfigManager::getSingleton().getFactions();
            for(Seat* seat : gameMap->getSeats())
            {
                // Rogue seat do not have to be configured
                if(seat->isRogueSeat())
                    continue;

                seat->setFaction(factions[seat->getConfigFactionIndex()]);

                int seatId = seat->getId();
                switch(seat->getConfigPlayerId())
                {
                    case 0:
                    {
                        // It is an inactive player
                        Player* inactivePlayer = new Player(gameMap, 0);
                        inactivePlayer->setNick("Inactive AI " + Helper::toString(seatId));
                        gameMap->addPlayer(inactivePlayer);
                        seat->setPlayer(inactivePlayer);
                        break;
                    }
                    case 1:
                    {
                        // It is an AI
                        // We set player id = 0 for AI players. ID is only used during seat configuration phase
                        // During the game, one should use the seat ID to identify a player
                        Player* aiPlayer = new Player(gameMap, 0);
                        aiPlayer->setNick("Keeper AI " + Helper::toString(seatId));
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
                               (client->getPlayer()->getId() == seat->getConfigPlayerId()))
                            {
                                seat->setPlayer(client->getPlayer());
                                gameMap->addPlayer(client->getPlayer());
                                break;
                            }
                        }
                        break;
                    }
                }
                seat->setTeamId(seat->getConfigTeamId());
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
                packetSend << ServerNotificationType::clientRejected;
                for(ODSocketClient* client : clientsToRemove)
                {
                    Player* player = client->getPlayer();
                    OD_LOG_INF("Rejecting player id="
                        + Helper::toString(player->getId())
                        + ", nick=" + player->getNick());
                    client->setState("rejected");
                    client->send(packetSend);
                    delete player;
                    client->setPlayer(nullptr);
                }
            }

            ODPacket packetSend;
            packetSend << ServerNotificationType::clientAccepted << ODApplication::turnsPerSecond;
            const std::vector<Player*>& players = gameMap->getPlayers();
            int32_t nbPlayers = players.size();
            packetSend << nbPlayers;
            for (Player* player : players)
            {
                packetSend << player->getNick() << player->getId()
                    << player->getSeat()->getId() << player->getSeat()->getTeamId();
                player->getSeat()->setMapSize(gameMap->getMapSizeX(), gameMap->getMapSizeY());
            }
            sendMsg(nullptr, packetSend);

            for (ODSocketClient* client : mSockClients)
            {
                if(!client->isConnected() || (client->getPlayer() == nullptr))
                    continue;

                ODPacket packetSend;
                int seatId = client->getPlayer()->getSeat()->getId();
                packetSend << ServerNotificationType::startGameMode << seatId << mServerMode;
                client->send(packetSend);
            }

            for(Seat* seat : gameMap->getSeats())
            {
                // We initialize the seats
                seat->initSeat();
            }

            mSeatsConfigured = true;
            gameMap->notifySeatsConfigured();
            break;
        }

        case ClientNotificationType::chat:
        {
            // TODO : handle chat for everybody/allies/player
            // As chat message do not interfere with GameMap, it is OK to send
            // them directly to the clients instead of queuing a ServerNotification
            // to the Server
            std::string chatMsg;
            OD_ASSERT_TRUE(packetReceived >> chatMsg);
            int32_t seatId = -1;
            if(clientSocket->getPlayer()->getSeat() != nullptr)
                seatId = clientSocket->getPlayer()->getSeat()->getId();

            ODPacket packetSend;
            const std::string& playerNick = clientSocket->getPlayer()->getNick();
            ServerNotification notif(ServerNotificationType::chat, nullptr);
            notif.mPacket << playerNick << chatMsg << seatId;
            sendAsyncMsg(notif);
            break;
        }

        case ClientNotificationType::askEntityPickUp:
        {
            std::string entityName;
            GameEntityType entityType;
            OD_ASSERT_TRUE(packetReceived >> entityType >> entityName);

            Player *player = clientSocket->getPlayer();
            GameEntity* entity = gameMap->getEntityFromTypeAndName(entityType, entityName);
            if(entity == nullptr)
            {
                OD_LOG_ERR("entityType=" + Helper::toString(static_cast<int32_t>(entityType)) + ", entityName=" + entityName);
                break;
            }
            bool allowPickup = entity->tryPickup(player->getSeat());
            if(!allowPickup)
            {
                OD_LOG_INF("player=" + player->getNick()
                        + " could not pickup entity entityType="
                        + Helper::toString(static_cast<int32_t>(entityType))
                        + ", entityName=" + entityName);
                break;
            }

            player->pickUpEntity(entity);
            break;
        }

        case ClientNotificationType::askHandDrop:
        {
            Player *player = clientSocket->getPlayer();
            Tile* tile = gameMap->tileFromPacket(packetReceived);
            if(tile == nullptr)
            {
                OD_LOG_ERR("player seatId=" + Helper::toString(player->getSeat()->getId())
                    + " send wrong tile");
                break;
            }
            if(!player->isDropHandPossible(tile, 0))
            {
                OD_LOG_ERR("player seatId=" + Helper::toString(player->getSeat()->getId())
                    + " could not drop entity in hand on tile "
                    + Tile::displayAsString(tile));
                break;
            }
            player->dropHand(tile, 0);
            break;
        }

        case ClientNotificationType::askPickupWorker:
        {
            Player *player = clientSocket->getPlayer();
            Creature* creature = gameMap->getWorkerToPickupBySeat(player->getSeat());
            if(creature == nullptr)
                break;

            player->pickUpEntity(creature);
            break;
        }

        case ClientNotificationType::askPickupFighter:
        {
            Player *player = clientSocket->getPlayer();
            Creature* creature = gameMap->getFighterToPickupBySeat(player->getSeat());
            if(creature == nullptr)
                break;

            player->pickUpEntity(creature);
            break;
        }

        case ClientNotificationType::askMarkTiles:
        {
            int x1, y1, x2, y2;
            bool isDigSet;
            Player* player = clientSocket->getPlayer();

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2 >> isDigSet);
            std::vector<Tile*> tiles = gameMap->rectangularRegion(x1, y1, x2, y2);
            player->markTilesForDigging(isDigSet, tiles, true);

            break;
        }

        case ClientNotificationType::askSlapEntity:
        {
            GameEntityType entityType;
            std::string entityName;
            Player* player = clientSocket->getPlayer();
            OD_ASSERT_TRUE(packetReceived >> entityType >> entityName);
            GameEntity* entity = gameMap->getEntityFromTypeAndName(entityType, entityName);
            if(entity == nullptr)
            {
                OD_LOG_WRN("entityType=" + Helper::toString(static_cast<int32_t>(entityType)) + ", entityName=" + entityName);
                break;
            }

            if(!entity->canSlap(player->getSeat()))
            {
                OD_LOG_INF("player seatId=" + Helper::toString(player->getSeat()->getId())
                    + " could not slap entity entityType="
                    + Helper::toString(static_cast<int32_t>(entityType))
                    + ", entityName=" + entityName);
                break;
            }

            OD_LOG_INF("player seatId=" + Helper::toString(player->getSeat()->getId()) + " slapped entity " + entity->getName());
            entity->slap();

            ServerNotification notif(ServerNotificationType::entitySlapped, player);
            sendAsyncMsg(notif);
            break;
        }

        case ClientNotificationType::askBuildRoom:
        {
            RoomType type;

            OD_ASSERT_TRUE(packetReceived >> type);
            Player* player = clientSocket->getPlayer();

            // We check if the room is available. It is not normal to receive a message
            // asking to build an unbuildable room since the client should only display
            // available rooms
            if(!SkillManager::isRoomAvailable(type, player->getSeat()))
            {
                OD_LOG_INF("WARNING: player seatId=" + Helper::toString(player->getSeat()->getId())
                    + " asked to build a room not available: " + RoomManager::getRoomNameFromRoomType(type));
                break;
            }

            if(!RoomManager::buildRoom(gameMap, type, player, packetReceived))
            {
                OD_LOG_INF("WARNING: player seatId=" + Helper::toString(player->getSeat()->getId())
                    + " couldn't build room: " + RoomManager::getRoomNameFromRoomType(type));
                break;
            }
            break;
        }

        case ClientNotificationType::askSellRoomTiles:
        {
            Player* player = clientSocket->getPlayer();
            RoomManager::sellRoomTiles(gameMap, player, packetReceived);
            break;
        }

        case ClientNotificationType::editorAskDestroyRoomTiles:
        {
            if(mServerMode != ServerMode::ModeEditor)
            {
                OD_LOG_ERR("Received editor command while wrong mode mode"
                    + Helper::toString(static_cast<int>(mServerMode)));
                break;
            }

            RoomManager::sellRoomTilesEditor(gameMap, packetReceived);
            break;
        }

        case ClientNotificationType::askBuildTrap:
        {
            TrapType type;

            OD_ASSERT_TRUE(packetReceived >> type);
            Player* player = clientSocket->getPlayer();

            // We check if the trap is available. It is not normal to receive a message
            // asking to build an unbuildable trap since the client should only display
            // available rooms
            if(!SkillManager::isTrapAvailable(type, player->getSeat()))
            {
                OD_LOG_INF("WARNING: player seatId=" + Helper::toString(player->getSeat()->getId())
                    + " asked to build a trap not available: " + TrapManager::getTrapNameFromTrapType(type));
                break;
            }

            if(!TrapManager::buildTrap(gameMap, type, player, packetReceived))
            {
                OD_LOG_INF("WARNING: player seatId=" + Helper::toString(player->getSeat()->getId())
                    + " couldn't build trap: " + TrapManager::getTrapNameFromTrapType(type));
                break;
            }

            // If the player is human and do not own a workshop, we warn him
            if(!player->getIsHuman())
                break;

            std::vector<Room*> rooms = gameMap->getRoomsByTypeAndSeat(RoomType::workshop, player->getSeat());
            if(!rooms.empty())
                break;

            ServerNotification *serverNotification = new ServerNotification(
                ServerNotificationType::chatServer, player);

            std::string msg = "You need a workshop to craft the trap!";
            serverNotification->mPacket << msg << EventShortNoticeType::genericGameInfo;
            ODServer::getSingleton().queueServerNotification(serverNotification);
            break;
        }

        case ClientNotificationType::askCastSpell:
        {
            SpellType spellType;

            OD_ASSERT_TRUE(packetReceived >> spellType);
            Player* player = clientSocket->getPlayer();

            // We check if the spell is available. It is not normal to receive a message
            // asking to cast an uncastable spell since the client should only display
            // available spells
            if(!SkillManager::isSpellAvailable(spellType, player->getSeat()))
            {
                OD_LOG_WRN("player " + player->getNick()
                    + " asked to cast a spell not available: " + SpellManager::getSpellNameFromSpellType(spellType));
                break;
            }

            uint32_t cooldown = player->getSpellCooldownTurns(spellType);
            if(cooldown > 0)
            {
                OD_LOG_WRN("player " + player->getNick()
                    + " asked to cast a spell " + SpellManager::getSpellNameFromSpellType(spellType) + " before end of cooldown: "
                    + Helper::toString(cooldown));
                break;
            }

            OD_LOG_INF("Player id: " + Helper::toString(player->getSeat()->getId()) + " casts spell " + SpellManager::getSpellNameFromSpellType(spellType));

            if(!SpellManager::castSpell(gameMap, spellType, player, packetReceived))
                break;

            uint32_t newCooldown = SpellManager::getSpellCooldown(spellType);
            player->setSpellCooldownTurns(spellType, newCooldown);
            break;
        }

        case ClientNotificationType::askSellTrapTiles:
        {
            Player* player = clientSocket->getPlayer();
            TrapManager::sellTrapTiles(gameMap, player->getSeat(), packetReceived);
            break;
        }

        case ClientNotificationType::askSetPlayerSettings:
        {
            Seat* playerSeat = clientSocket->getPlayer()->getSeat();
            bool koCreatures;
            OD_ASSERT_TRUE(packetReceived >> koCreatures);
            playerSeat->setPlayerSettings(koCreatures);
            break;
        }

        case ClientNotificationType::editorAskDestroyTrapTiles:
        {
            if(mServerMode != ServerMode::ModeEditor)
            {
                OD_LOG_ERR("Received editor command while wrong mode mode"
                    + Helper::toString(static_cast<int>(mServerMode)));
                break;
            }

            TrapManager::sellTrapTilesEditor(gameMap, packetReceived);
            break;
        }

        case ClientNotificationType::ackNewTurn:
        {
            int64_t turn;
            OD_ASSERT_TRUE(packetReceived >> turn);
            clientSocket->setLastTurnAck(turn);
            break;
        }

        case ClientNotificationType::askCreatureInfos:
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

        case ClientNotificationType::askSaveMap:
        {
            Player* player = clientSocket->getPlayer();
            // Only the player allowed to configure the game can save games
            if(mPlayerConfig != player)
                break;

            // We only allow to save game if launching in client+server mode
            if(ODClient::getSingletonPtr() == nullptr)
                break;
            if(!ODClient::getSingleton().isConnected())
                break;

            const boost::filesystem::path levelPath(gameMap->getLevelFileName());
            std::string fileLevel = levelPath.filename().string();
            // In editor mode, we don't allow a player to have creatures in hand while saving map
            if((mServerMode == ServerMode::ModeEditor) &&
                (player->numObjectsInHand() > 0))
            {
                // We cannot save the map
                std::string msg = "Map could not be saved because player hand is not empty";
                ServerNotification notif(ServerNotificationType::chatServer, player);
                notif.mPacket << msg << EventShortNoticeType::genericGameInfo;
                sendAsyncMsg(notif);
                break;
            }

            boost::filesystem::path levelSave;
            if(mServerMode == ServerMode::ModeEditor)
            {
                // In editor mode, we save in the original folder
                levelSave = levelPath;

                // If the level was not a custom one, we save it as a custom one now.
                // Note: We don't compare for official levels path, as they may be relative and unreliable.
                std::string levelStr = levelSave.string();
                ResourceManager& resMgr = ResourceManager::getSingleton();
                bool skirmishLevelType = (levelStr.find("skirmish") != std::string::npos);
                if (skirmishLevelType) {
                    if (levelStr.find(resMgr.getUserLevelPathSkirmish()) == std::string::npos) {
                        levelSave = boost::filesystem::path(resMgr.getUserLevelPathSkirmish() + fileLevel);
                    }
                }
                else if (levelStr.find(resMgr.getUserLevelPathMultiplayer()) == std::string::npos) {
                    levelSave = boost::filesystem::path(resMgr.getUserLevelPathMultiplayer() + fileLevel);
                }
                std::cout << levelSave.string() << std::endl;
            }
            else
            {
                // We save in the saved game folder
                static std::locale loc(std::wcout.getloc(), new boost::posix_time::time_facet("%Y-%m-%d_%H%M%S"));

                std::ostringstream ss;
                ss.imbue(loc);
                ss << boost::posix_time::second_clock::local_time() << "-";
                switch(mServerMode)
                {
                    case ServerMode::ModeGameSinglePlayer:
                        ss << SAVEGAME_SKIRMISH_PREFIX;
                        ss << fileLevel;
                        break;
                    case ServerMode::ModeGameMultiPlayer:
                        ss << SAVEGAME_MULTIPLAYER_PREFIX;
                        ss << fileLevel;
                        break;
                    case ServerMode::ModeGameLoaded:
                    {
                        // We look for the Skirmish or multiplayer prefix and keep it.
                        uint32_t indexSk = fileLevel.find(SAVEGAME_SKIRMISH_PREFIX);
                        uint32_t indexMp = fileLevel.find(SAVEGAME_MULTIPLAYER_PREFIX);
                        if((indexSk != std::string::npos) && (indexMp == std::string::npos))
                        {
                            // Skirmish savegame
                            ss << SAVEGAME_SKIRMISH_PREFIX;
                            ss << fileLevel.substr(indexSk + SAVEGAME_SKIRMISH_PREFIX.length());

                        }
                        else if((indexSk == std::string::npos) && (indexMp != std::string::npos))
                        {
                            // Multiplayer savegame
                            ss << SAVEGAME_MULTIPLAYER_PREFIX;
                            ss << fileLevel.substr(indexMp + SAVEGAME_MULTIPLAYER_PREFIX.length());
                        }
                        else if((indexSk != std::string::npos) && (indexMp != std::string::npos))
                        {
                            // We found both prefixes. That can happen if the name contains the other
                            // prefix. Because of filename construction, we know that the lowest is the good
                            if(indexSk < indexMp)
                            {
                                ss << SAVEGAME_SKIRMISH_PREFIX;
                                ss << fileLevel.substr(indexSk + SAVEGAME_SKIRMISH_PREFIX.length());
                            }
                            else
                            {
                                ss << SAVEGAME_MULTIPLAYER_PREFIX;
                                ss << fileLevel.substr(indexMp + SAVEGAME_MULTIPLAYER_PREFIX.length());
                            }
                        }
                        else
                        {
                            // We couldn't find any prefix. That's not normal
                            OD_LOG_ERR("fileLevel=" + fileLevel);
                            ss << fileLevel;
                        }
                        break;
                    }
                    default:
                        OD_LOG_ERR("mode=" + Helper::toString(static_cast<int>(mServerMode)));
                        ss << fileLevel;
                        break;
                }
                std::string savePath = ResourceManager::getSingleton().getSaveGamePath() + ss.str();
                levelSave = boost::filesystem::path(savePath);
            }

            // If the file exists, we make a backup
            if (boost::filesystem::exists(levelSave))
                boost::filesystem::rename(levelSave, levelSave.string() + ".bak");

            std::string msg = "Map saved successfully as: " + levelSave.string();
            if (!MapHandler::writeGameMapToFile(levelSave.string(), *gameMap))
            {
                msg = "Couldn't not save map file as: " + levelSave.string() + "\nPlease check logs.";
            }
            // We notify all the players that the game was saved successfully
            ServerNotification notif(ServerNotificationType::chatServer, nullptr);
            notif.mPacket << msg << EventShortNoticeType::genericGameInfo;
            sendAsyncMsg(notif);
            break;
        }

        case ClientNotificationType::editorAskChangeTiles:
        {
            if(mServerMode != ServerMode::ModeEditor)
            {
                OD_LOG_ERR("Received editor command while wrong mode mode" + Helper::toString(static_cast<int>(mServerMode)));
                break;
            }
            int x1, y1, x2, y2;
            TileType tileType;
            double tileFullness;
            int seatId;

            OD_ASSERT_TRUE(packetReceived >> x1 >> y1 >> x2 >> y2 >> tileType >> tileFullness >> seatId);
            std::vector<Tile*> selectedTiles = gameMap->rectangularRegion(x1, y1, x2, y2);
            std::vector<Tile*> affectedTiles;
            Seat* seat = nullptr;
            if(seatId != -1)
                seat = gameMap->getSeatById(seatId);

            for(Tile* tile : selectedTiles)
            {
                // We do not change tiles where there is something
                if((tile->numEntitiesInTile() > 0) &&
                   ((tileFullness > 0.0) || (tileType == TileType::lava) || (tileType == TileType::water)))
                    continue;
                if(tile->getCoveringBuilding() != nullptr)
                    continue;

                affectedTiles.push_back(tile);
                tile->setType(tileType);
                tile->setFullness(tileFullness);
                if(seat != nullptr)
                    tile->claimTile(seat);
                else
                    tile->unclaimTile();

                tile->computeTileVisual();
            }
            if(!affectedTiles.empty())
            {
                uint32_t nbTiles = affectedTiles.size();
                const std::vector<Seat*>& seats = gameMap->getSeats();
                for(Seat* seat : seats)
                {
                    if(seat->getPlayer() == nullptr)
                        continue;
                    if(!seat->getPlayer()->getIsHuman())
                        continue;

                    ServerNotification notif(ServerNotificationType::refreshTiles, seat->getPlayer());
                    notif.mPacket << nbTiles;
                    for(Tile* tile : affectedTiles)
                    {
                        gameMap->tileToPacket(notif.mPacket, tile);
                        seat->updateTileStateForSeat(tile);
                        tile->exportToPacketForUpdate(notif.mPacket, seat);

                    }
                    sendAsyncMsg(notif);
                }
            }
            break;
        }

        case ClientNotificationType::editorAskBuildRoom:
        {
            if(mServerMode != ServerMode::ModeEditor)
            {
                OD_LOG_ERR("Received editor command while wrong mode mode"
                    + Helper::toString(static_cast<int>(mServerMode)));
                break;
            }
            RoomType type;

            OD_ASSERT_TRUE(packetReceived >> type);

            Player* player = clientSocket->getPlayer();
            if(!RoomManager::buildRoomEditor(gameMap, type, packetReceived))
            {
                OD_LOG_INF("WARNING: player seatId=" + Helper::toString(player->getSeat()->getId())
                    + " couldn't build room: " + RoomManager::getRoomNameFromRoomType(type));
                break;
            }
            break;
        }

        case ClientNotificationType::editorAskBuildTrap:
        {
            if(mServerMode != ServerMode::ModeEditor)
            {
                OD_LOG_ERR("Received editor command while wrong mode mode"
                    + Helper::toString(static_cast<int>(mServerMode)));
                break;
            }
            TrapType type;

            OD_ASSERT_TRUE(packetReceived >> type);

            Player* player = clientSocket->getPlayer();
            if(!TrapManager::buildTrapEditor(gameMap, type, packetReceived))
            {
                OD_LOG_INF("WARNING: player seatId=" + Helper::toString(player->getSeat()->getId())
                    + " couldn't build trap: " + TrapManager::getTrapNameFromTrapType(type));
                break;
            }
            break;
        }

        case ClientNotificationType::editorCreateWorker:
        {
            if(mServerMode != ServerMode::ModeEditor)
            {
                OD_LOG_ERR("Received editor command while wrong mode mode" + Helper::toString(static_cast<int>(mServerMode)));
                break;
            }
            Player* player = clientSocket->getPlayer();
            int seatId;
            OD_ASSERT_TRUE(packetReceived >> seatId);
            Seat* seatCreature = gameMap->getSeatById(seatId);
            if(seatCreature == nullptr)
            {
                OD_LOG_ERR("seatId=" + Helper::toString(seatId));
                break;
            }

            const CreatureDefinition *classToSpawn = ConfigManager::getSingleton().getCreatureDefinitionDefaultWorker();
            if(classToSpawn == nullptr)
            {
                OD_LOG_ERR("unexpected null classToSpawn for getCreatureDefinitionDefaultWorker");
                break;
            }
            Creature* newCreature = new Creature(gameMap, true, classToSpawn, seatCreature);
            newCreature->addToGameMap();
            newCreature->setPosition(Ogre::Vector3(0.0, 0.0, 0.0));
            // In editor mode, every player has vision
            for(Seat* seat : gameMap->getSeats())
            {
                if(seat->getPlayer() == nullptr)
                    continue;
                if(!seat->getPlayer()->getIsHuman())
                    continue;

                newCreature->addSeatWithVision(seat, true);
            }

            player->pickUpEntity(newCreature);
            break;
        }

        case ClientNotificationType::editorCreateFighter:
        {
            if(mServerMode != ServerMode::ModeEditor)
            {
                OD_LOG_ERR("Received editor command while wrong mode mode" + Helper::toString(static_cast<int>(mServerMode)));
                break;
            }
            Player* player = clientSocket->getPlayer();
            int seatId;
            std::string className;
            OD_ASSERT_TRUE(packetReceived >> seatId >> className);
            Seat* seatCreature = gameMap->getSeatById(seatId);
            if(seatCreature == nullptr)
            {
                OD_LOG_ERR("seatId=" + Helper::toString(seatId));
                break;
            }
            const CreatureDefinition *classToSpawn = gameMap->getClassDescription(className);
            if(classToSpawn == nullptr)
            {
                OD_LOG_ERR("Couldn't spawn creature class=" + className);
                break;
            }
            Creature* newCreature = new Creature(gameMap, true, classToSpawn, seatCreature);
            newCreature->addToGameMap();
            newCreature->setPosition(Ogre::Vector3(0.0, 0.0, 0.0));
            // In editor mode, every player has vision
            for(Seat* seat : gameMap->getSeats())
            {
                if(seat->getPlayer() == nullptr)
                    continue;
                if(!seat->getPlayer()->getIsHuman())
                    continue;

                newCreature->addSeatWithVision(seat, true);
            }

            player->pickUpEntity(newCreature);
            break;
        }

        case ClientNotificationType::askSetSkillTree:
        {
            Player* player = clientSocket->getPlayer();
            uint32_t nbItems;
            OD_ASSERT_TRUE(packetReceived >> nbItems);
            std::vector<SkillType> skills;
            while(nbItems > 0)
            {
                nbItems--;
                SkillType skill;
                OD_ASSERT_TRUE(packetReceived >> skill);
                skills.push_back(skill);
            }

            player->getSeat()->setSkillTree(skills);
            break;
        }

        case ClientNotificationType::askExecuteConsoleCommand:
        {
            uint32_t nbArgs;
            std::string str;
            std::vector<std::string> args;
            OD_ASSERT_TRUE(packetReceived >> nbArgs);
            while(nbArgs > 0)
            {
                --nbArgs;
                OD_ASSERT_TRUE(packetReceived >> str);
                args.push_back(str);
            }
            Player* player = clientSocket->getPlayer();
            handleConsoleCommand(player, gameMap, args);
            break;
        }

        case ClientNotificationType::editorAskCreateMapLight:
        {
            Player* player = clientSocket->getPlayer();
            MapLight* mapLight = new MapLight(gameMap, true);
            mapLight->setName(gameMap->nextUniqueNameMapLight());
            mapLight->addToGameMap();
            mapLight->setPosition(Ogre::Vector3(0.0, 0.0, 3.75));
            // In editor mode, every player has vision
            for(Seat* seat : gameMap->getSeats())
            {
                if(seat->getPlayer() == nullptr)
                    continue;
                if(!seat->getPlayer()->getIsHuman())
                    continue;

                mapLight->addSeatWithVision(seat, true);
            }
            player->pickUpEntity(mapLight);
            break;
        }

        default:
        {
            OD_LOG_ERR("Unhandled command received from client:"
                + Helper::toString(static_cast<int>(clientCommand)));
        }
    }

    return true;
}

ODSocketClient* ODServer::notifyNewConnection(sf::TcpListener& sockListener)
{
    ODSocketClient* newClient = new ODSocketClient;
    sf::Socket::Status status = sockListener.accept(newClient->getSockClient());
    if (status != sf::Socket::Done)
    {
        OD_LOG_ERR("Error while listening to socket status=" + Helper::toString(static_cast<uint32_t>(status)));
        delete newClient;
        return nullptr;
    }

    switch(mServerState)
    {
        case ServerState::StateNone:
        {
            // It is not normal to receive new connexions while not connected. We are in an unexpected state
            OD_LOG_ERR("Unexpected none server mode");
            delete newClient;
            return nullptr;
        }
        case ServerState::StateConfiguration:
        {
            newClient->setState("connected");
            return newClient;
        }
        case ServerState::StateGame:
        {
            // TODO : handle re-connexion if a client was disconnected and tries to reconnect
            OD_LOG_WRN("Received a reconnexion from a client while in game state");
            delete newClient;
            return nullptr;
        }
        default:
            OD_LOG_ERR("Unexpected server state=" + Helper::toString(static_cast<uint32_t>(mServerState)));
            break;
    }

    delete newClient;
    return nullptr;
}

bool ODServer::notifyClientMessage(ODSocketClient *clientSocket)
{
    bool ret = processClientNotifications(clientSocket);
    if(!ret)
    {
        std::string nick = clientSocket->getPlayer() ? clientSocket->getPlayer()->getNick() : std::string();
        std::string message = nick.empty() ?
                              "Client disconnected state=" + clientSocket->getState() :
                              "Client (" + nick + ") disconnected state=" + clientSocket->getState();
        OD_LOG_INF(message);
        if(std::string("ready").compare(clientSocket->getState()) == 0)
        {
            for(Player* player : mGameMap->getPlayers())
            {
                if(!player->getIsHuman())
                    continue;

                ServerNotification *serverNotification = new ServerNotification(
                    ServerNotificationType::chatServer, player);
                std::string msg = nick.empty() ?
                                  "A client disconnected." :
                                  nick + " disconnected.";
                serverNotification->mPacket << msg << EventShortNoticeType::genericGameInfo;
                queueServerNotification(serverNotification);
            }
        }

        if(mSeatsConfigured)
        {
            mDisconnectedPlayers.push_back(clientSocket->getPlayer());
        }
        // TODO : wait at least 1 minute if the client reconnects if deconnexion happens during game
    }
    return ret;
}

void ODServer::stopServer()
{
    // We start by stopping server to make sure no new message comes
    ODSocketServer::stopServer();

    mServerState = ServerState::StateNone;
    mSeatsConfigured = false;
    mDisconnectedPlayers.clear();
    mPlayerConfig = nullptr;

    // Now that the server is stopped, we can remove all pending messages
    while(!mServerNotificationQueue.empty())
    {
        delete mServerNotificationQueue.front();
        mServerNotificationQueue.pop_front();
    }
    mGameMap->clearAll();
}

void ODServer::notifyExit()
{
    while(!mServerNotificationQueue.empty())
    {
        delete mServerNotificationQueue.front();
        mServerNotificationQueue.pop_front();
    }

    ServerNotification* exitServerNotification = new ServerNotification(
        ServerNotificationType::exit, nullptr);
    queueServerNotification(exitServerNotification);
}

ODSocketClient* ODServer::getClientFromPlayer(Player* player)
{
    for (ODSocketClient* client : mSockClients)
    {
        if(client->getPlayer() == player)
            return client;
    }

    return nullptr;
}

ODSocketClient* ODServer::getClientFromPlayerId(int32_t playerId)
{
    for (ODSocketClient* client : mSockClients)
    {
        if(client->getPlayer()->getId() == playerId)
            return client;
    }

    return nullptr;
}

bool ODServer::waitEndGame()
{
    // If the server is not launched, we don't allow to wait for end of game
    if(mServerState == ServerState::StateNone)
        return false;

    mThread->wait();
    return true;
}

void ODServer::fireSeatConfigurationRefresh()
{
    ODPacket packetSend;
    packetSend << ServerNotificationType::seatConfigurationRefresh;
    for(Seat* seat : mGameMap->getSeats())
    {
        // Rogue seat do not have to be configured
        if(seat->isRogueSeat())
            continue;

        int seatId = seat->getId();
        packetSend << seatId;

        int32_t factionIndex = seat->getConfigFactionIndex();
        if(factionIndex == -1)
        {
            packetSend << false;
        }
        else
        {
            packetSend << true;
            packetSend << factionIndex;
        }

        int32_t playerId = seat->getConfigPlayerId();
        if(playerId == -1)
        {
            packetSend << false;
        }
        else
        {
            packetSend << true;
            packetSend << playerId;
        }

        int32_t teamId = seat->getConfigTeamId();
        if(teamId == -1)
        {
            packetSend << false;
        }
        else
        {
            packetSend << true;
            packetSend << teamId;
        }
    }
    sendMsg(nullptr, packetSend);
}

int32_t ODServer::getNetworkPort() const
{
    int32_t port = ResourceManager::getSingleton().getForcedNetworkPort();
    if(port != -1)
        return port;

    return ConfigManager::getSingleton().getNetworkPort();
}

void ODServer::printConsoleMsg(const std::string& text)
{
    OD_LOG_INF("Console:" + text);
}

ODPacket& operator<<(ODPacket& os, const EventShortNoticeType& type)
{
    os << static_cast<int32_t>(type);
    return os;
}

ODPacket& operator>>(ODPacket& is, EventShortNoticeType& type)
{
    int32_t tmp;
    OD_ASSERT_TRUE(is >> tmp);
    type = static_cast<EventShortNoticeType>(tmp);
    return is;
}
