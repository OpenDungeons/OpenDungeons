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

#ifndef ODCLIENT_H
#define ODCLIENT_H

#include "network/ODSocketClient.h"
#include "network/ClientNotification.h"

#include <OgreSingleton.h>

#include <deque>

class GameMap;
class ODPacket;
class ChatMessage;
class EventMessage;

class ODClient: public Ogre::Singleton<ODClient>,
    public ODSocketClient
{
    friend class Console;
 public:
    ODClient();
    ~ODClient();

    // CLIENT

    /*! \brief The function which monitors the clientNotificationQueue for new events and informs the server about them.
     *
     * This runs on the client side and acts as a "consumer" on the
     * clientNotificationQueue.  It takes an event out of the queue, determines
     * which clients need to be informed about that particular event, and
     * dispacthes TCP packets to inform the clients about the new information.
     */
    void processClientNotifications();

    //! \brief Connects to the server host:port
    bool connect(const std::string& host, const int port, uint32_t timeout, const std::string& outputReplayFilename) override;

    //! \brief Connects to the server host:port
    bool replay(const std::string& filename);

    //! \brief Adds a client notification to the client notification queue.
    void queueClientNotification(ClientNotification* n);

    /*! \brief Adds a client notification to the client notification queue.
     *  \param type The type of the notification
     *  \param args The arguments that are to be piped into the notification.
     */
    template<typename ...Args>
    void queueClientNotification(ClientNotificationType type, const Args&... args);

    /*! \brief Adds a client notification to the client notification queue.
     *  \param type The type of the notification
     */
    void queueClientNotification(ClientNotificationType type)
    {
        mClientNotificationQueue.emplace_back(new ClientNotification(type));
    }

    //! \brief Disconnect the client.
    //! \param keepReplay Tells whether to keep the new replay file.
    void disconnect(bool keepReplay = false);

    //! \brief Adds a client notification to the client notification queue.
    void notifyExit();

    const std::string& getLevelFilename() {return mLevelFilename;}

    inline bool getIsPlayerConfig() const
    { return mIsPlayerConfig; }

 protected:
    bool processMessage(ServerNotificationType cmd, ODPacket& packetReceived) override;
    void playerDisconnected() override;

 private:
    //! \brief Convenience function to send a game event.
    void addEventMessage(EventMessage* event);

    //! \brief Refreshes the player's goals + main data
    void refreshMainUI(const std::string& goalsString);

    std::string mTmpReceivedString;
    std::string mLevelFilename;

    std::deque<ClientNotification*> mClientNotificationQueue;

    // true if the server told us we are allowed to configure the game. False otherwise
    bool mIsPlayerConfig;

};

template<typename ...Args>
void ODClient::queueClientNotification(ClientNotificationType type, const Args&... args)
{
    queueClientNotification(type);
    ODPacket::putInPacket(mClientNotificationQueue.back()->mPacket, args...);
}

#endif // ODCLIENT_H
