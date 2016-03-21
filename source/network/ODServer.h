/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#ifndef ODSERVER_H
#define ODSERVER_H

#include "ODSocketServer.h"
#include "modes/ConsoleInterface.h"

#include <OgreSingleton.h>

class ServerNotification;
class GameMap;

enum class ServerMode;

//! \brief An enum used to know what kind of game event it is.
enum class EventShortNoticeType : int32_t
{
    genericGameInfo,
    majorGameEvent,
    aboutCreatures,
    aboutSkills,
    aboutObjectives
};

ODPacket& operator<<(ODPacket& os, const EventShortNoticeType& type);
ODPacket& operator>>(ODPacket& is, EventShortNoticeType& type);

/**
 * When playing single player or multiplayer, there is always one reference gamemap. It is
 * the one on ODServer. There is also a client gamemap in ODFrameListener which is supposed to be
 * a "copy" from the server gamemap containing only the useful information for the player.
 * The server has its own thread to update the server gamemap. When a relevant change occurs,
 * the server sends messages to the clients so that they know they should update the client gamemaps.
 * The server gamemap should only be accessed from the server thread because it is not thread safe.
 * and the server thread should never be used for calling functions from the client gamemap.
 * As a consequence, ODFrameListener.h should not be included is ODServer.cpp
 * Interactions between client and server gamemaps should only occur through client/server messages
 * by queuing messages (calling queueServerNotification).
 * Moreover, in processServerNotifications, no message should be sent directly to the clients (by
 * creating an ODPacket and sending it to the clients) because it would break the queue order. Instead,
 * queueServerNotification should be called with the message.
 * Note that this rule is not followed when dealing with client connexions or chat because there
 * is no need to synchronize such messages with the gamemap.
 */
class ODServer: public Ogre::Singleton<ODServer>,
    public ODSocketServer
{
 public:
     enum ServerState
     {
         StateNone,
         StateConfiguration,
         StateGame
     };
    ODServer();
    virtual ~ODServer();

    inline ServerMode getServerMode() const
    { return mServerMode; }

    bool startServer(const std::string& creator, const std::string& levelFilename, ServerMode mode, bool useMasterServer);
    void stopServer();

    //! \brief Adds a server notification to the server notification queue. The message will be sent to the concerned player
    void queueServerNotification(ServerNotification* n);

    //! \brief Sends an asynchronous message to the concerned player. This function should be used really carefully as it can easily
    //! make the game crash by sending messages in an unexpected order (changing the state of an entity that was not created, for example).
    //! In most of the can, we will use it for messages that do not need synchronization with the game (example : chat) or
    //! for messages that need to show reactivity (after a player does something like building a room or tried to pickup a creature).
    void sendAsyncMsg(ServerNotification& notif);

    void notifyExit();

    //! This function will block the calling thread until the game is launched and
    //! all the clients disconnect. Then, it will return true if everything went well
    //! and false if there is an error (server not launched or system error)
    bool waitEndGame();

    int32_t getNetworkPort() const;

protected:
    ODSocketClient* notifyNewConnection(sf::TcpListener& sockListener) override;
    bool notifyClientMessage(ODSocketClient *sock) override;
    void serverThread() override;

private:
    uint32_t mUniqueNumberPlayer;
    ServerMode mServerMode;
    ServerState mServerState;
    GameMap *mGameMap;
    bool mSeatsConfigured;
    //! Player allowed to configure the lobby, save the game, ...
    Player* mPlayerConfig;
    std::vector<Player*> mDisconnectedPlayers;

    std::deque<ServerNotification*> mServerNotificationQueue;

    std::map<ODSocketClient*, std::vector<std::string>> mCreaturesInfoWanted;

    ConsoleInterface mConsoleInterface;

    std::string mMasterServerGameId;
    double mMasterServerGameStatusUpdateTime;

    void printConsoleMsg(const std::string& text);

    ODSocketClient* getClientFromPlayer(Player* player);
    ODSocketClient* getClientFromPlayerId(int32_t playerId);

    //! \brief Called when a new turn started.
    void startNewTurn(double timeSinceLastTurn);

    /*! \brief Monitors mServerNotificationQueue for new events and informs the clients about them.
     *
     * This function is used in server mode and acts as a "consumer" on
     * mServerNotificationQueue.  It takes an event out of the queue, determines
     * which clients need to be informed about that particular event, and
     * dispacthes TCP packets to inform the clients about the new information.
     */
    void processServerNotifications();

    /*! \brief The function running in server-mode which listens for messages from an individual, already connected, client.
     *
     * This function receives TCP packets one at a time from a connected client,
     * decodes them, and carries out requests for the client, returning any
     * results.
     * \returns false When the client has disconnected.
     */
    bool processClientNotifications(ODSocketClient* clientSocket);

    //! \brief Sends the packet to the given player. If player is nullptr, the packet is sent to every connected player
    void sendMsg(Player* player, ODPacket& packet);

    void fireSeatConfigurationRefresh();

    //! \brief Handles console command. player is the player that launched the command
    void handleConsoleCommand(Player* player, GameMap* gameMap, const std::vector<std::string>& args);
};

#endif // ODSERVER_H
