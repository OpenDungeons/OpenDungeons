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

#ifndef ODSERVER_H
#define ODSERVER_H

#include "ODSocketServer.h"
#include "rooms/Room.h"

#include <OgreSingleton.h>

class ServerNotification;
class GameMap;
class ODConsoleCommand;

/**
 * When playing single player or multiplayer, there is always one reference gamemap. It is
 * the one on ODServer. There is also a client gamemap in ODFrameListener which is supposed to be
 * a "copy" from the server gamemap containing only the usefull information for the player.
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
     enum ServerMode
     {
         ModeNone,
         ModeGameSinglePlayer,
         ModeGameMultiPlayer,
         ModeEditor
     };
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

    bool startServer(const std::string& levelFilename, ServerMode mode);
    void stopServer();

    //! \brief Adds a server notification to the server notification queue. The message will be sent to the concerned player
    void queueServerNotification(ServerNotification* n);

    //! \brief Sends an asynchronous message to the concerned player. This function should be used really carefully as it can easily
    //! make the game crash by sending messages in an unexpected order (changing the state of an entity that was not created, for example).
    //! In most of the can, we will use it for messages that do not need synchronization with the game (example : chat) or
    //! for messages that need to show reactivity (after a player does something like building a room or tried to pickup a creature).
    void sendAsyncMsg(ServerNotification& notif);

    //! \brief Adds a console command to the queue.
    void queueConsoleCommand(ODConsoleCommand* cc);

    void notifyExit();

    static const std::string SERVER_INFORMATION;
    friend ODPacket& operator<<(ODPacket& os, const ODServer::ServerMode& sm);
    friend ODPacket& operator>>(ODPacket& is, ODServer::ServerMode& sm);

protected:
    bool notifyNewConnection(ODSocketClient *sock);
    bool notifyClientMessage(ODSocketClient *sock);
    void serverThread();

private:
    ServerMode mServerMode;
    ServerState mServerState;
    GameMap *mGameMap;
    bool mSeatsConfigured;

    std::deque<ServerNotification*> mServerNotificationQueue;
    std::deque<ODConsoleCommand*> mConsoleCommandQueue;

    std::map<ODSocketClient*, std::vector<std::string>> mCreaturesInfoWanted;

    ODSocketClient* getClientFromPlayer(Player* player);

    //! \brief Called when a new turn started.
    void startNewTurn(double timeSinceLastFrame);

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
     * This function recieves TCP packets one at a time from a connected client,
     * decodes them, and carries out requests for the client, returning any
     * results.
     * \returns false When the client has disconnected.
     */
    bool processClientNotifications(ODSocketClient* clientSocket);

    void processServerCommandQueue();

    //! \brief Sends the packet to the given player. If player is nullptr, the packet is sent to every connected player
    void sendMsg(Player* player, ODPacket& packet);
};

#endif // ODSERVER_H
