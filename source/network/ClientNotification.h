/*!
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

#ifndef CLIENTNOTIFICATION_H
#define CLIENTNOTIFICATION_H

#include "network/ODPacket.h"

#include <deque>

/*! \brief A data structure used to pass messages to the clientNotificationProcessor thread.
 *
 */
class ClientNotification
{
    friend class ODClient;

public:
    enum ClientNotificationType
    {
        // Communication with server
        hello,
        levelOK, // Tells the server the level loading was ok.
        setNick,

        chat,

        // Notification in game
        askEntityPickUp,
        askHandDrop,
        askMarkTile,
        askBuildRoom,
        askSellRoomTiles,
        askBuildTrap,
        askSellTrapTiles,
        ackNewTurn,
        askCreatureInfos,
        askPickupWorker,
        askPickupFighter,

        //  Editor
        editorAskSaveMap,
        editorAskChangeTiles,
        editorAskBuildRoom,
        editorAskBuildTrap,
        editorAskDestroyRoomTiles,
        editorAskDestroyTrapTiles,
        editorCreateWorker,
        editorCreateFighter
    };

    ClientNotification(ClientNotificationType type);
    virtual ~ClientNotification()
    {}

    ODPacket mPacket;

    static std::string typeString(ClientNotificationType type);

    friend ODPacket& operator<<(ODPacket& os, const ClientNotification::ClientNotificationType& nt);
    friend ODPacket& operator>>(ODPacket& is, ClientNotification::ClientNotificationType& nt);

private:
    ClientNotificationType mType;
};

#endif // CLIENTNOTIFICATION_H
