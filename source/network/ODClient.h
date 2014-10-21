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

#ifndef ODCLIENT_H
#define ODCLIENT_H

#include "ClientNotification.h"
#include "ODSocketClient.h"

#include <OgreSingleton.h>

class GameMap;
class ODPacket;

class ODClient: public Ogre::Singleton<ODClient>,
    public ODSocketClient
{
    friend class Console;
 public:
    ODClient();
    ~ODClient();

    // CLIENT

    /*! \brief A function which runs on the client to handle communications with the server.
     *
     * A single instance of this thread is spawned by the client when it connects
     * to a server.
     */
    void processClientSocketMessages(GameMap& gameMap);

    /*! \brief The function which monitors the clientNotificationQueue for new events and informs the server about them.
     *
     * This runs on the client side and acts as a "consumer" on the
     * clientNotificationQueue.  It takes an event out of the queue, determines
     * which clients need to be informed about that particular event, and
     * dispacthes TCP packets to inform the clients about the new information.
     */
    void processClientNotifications();

    //! \brief Connects to the server host:port
    bool connect(const std::string& host, const int port);

    //! \brief Adds a client notification to the client notification queue.
    void queueClientNotification(ClientNotification* n);

    void disconnect();

    //! \brief Adds a client notification to the client notification queue.
    void notifyExit();

    const std::string& getLevelFilename() {return mLevelFilename;}

 private:
    bool processOneClientSocketMessage();

    void sendToServer(ODPacket& packetToSend);

    std::string mTmpReceivedString;
    std::string mLevelFilename;

    std::deque<ClientNotification*> mClientNotificationQueue;

};

#endif // ODCLIENT_H
