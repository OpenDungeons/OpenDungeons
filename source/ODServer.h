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

#include "ChatMessage.h"
#include "ODFrameListener.h"
#include "ODSocketServer.h"

#include <OgreSingleton.h>

class ServerNotification;
class Socket;

class ODServer: public Ogre::Singleton<ODServer>,
    public ODSocketServer
{
 public:
    ODServer();
    ~ODServer();

    bool startServer();

    //! \brief Adds a server notification to the server notification queue.
    void queueServerNotification(ServerNotification* n);

    //! \brief Process server events, such as server notifications and received messages.
    void processServerEvents();

    /*! \brief Monitors the serverNotificationQueue for new events and informs the clients about them.
     *
     * This function is used in server mode and acts as a "consumer" on the
     * serverNotificationQueue.  It takes an event out of the queue, determines
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

    // TODO : this has to be public for chat messages. When implementation will allow it, it should be private
    // TODO : check if it can be removed
    void sendToAllClients(ODPacket& packetSend);

    bool notifyNewConnection(ODSocketClient *sock);
    void notifyClientMessage(ODSocketClient *sock);


};

#endif // ODSERVER_H
