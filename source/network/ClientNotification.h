/*!
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

#ifndef CLIENTNOTIFICATION_H
#define CLIENTNOTIFICATION_H

#include "network/ODPacket.h"

#include <deque>

enum class ClientNotificationType
{
    // Communication with server
    hello,
    levelOK, // Tells the server the level loading was ok.
    setNick,
    readyForSeatConfiguration,
    // Messages that should be sent only by the client side of the server
    // (where the game configuration is done)
    seatConfigurationSet,
    seatConfigurationRefresh,

    chat,

    // Notification in game
    askEntityPickUp,
    askHandDrop,
    askMarkTiles,
    askBuildRoom,
    askSellRoomTiles,
    askBuildTrap,
    askSellTrapTiles,
    ackNewTurn,
    askCreatureInfos,
    askPickupWorker,
    askPickupFighter,
    askSlapEntity,
    askCastSpell,
    askSetSkillTree,
    askSetPlayerSettings,

    askSaveMap,
    askExecuteConsoleCommand,

    //  Editor
    editorAskChangeTiles,
    editorAskBuildRoom,
    editorAskBuildTrap,
    editorAskDestroyRoomTiles,
    editorAskDestroyTrapTiles,
    editorCreateWorker,
    editorCreateFighter,
    editorAskCreateMapLight
};

ODPacket& operator<<(ODPacket& os, const ClientNotificationType& nt);
ODPacket& operator>>(ODPacket& is, ClientNotificationType& nt);

/*! \brief A data structure used to pass messages to the clientNotificationProcessor thread.
 *
 */
class ClientNotification
{
    friend class ODClient;

public:
    ClientNotification(ClientNotificationType type);
    virtual ~ClientNotification()
    {}

    ODPacket mPacket;

    static std::string typeString(ClientNotificationType type);

private:
    ClientNotificationType mType;
};

#endif // CLIENTNOTIFICATION_H
