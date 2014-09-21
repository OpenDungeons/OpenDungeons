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

#include "ClientNotification.h"
#include "LogManager.h"

#include <OgreStringConverter.h>

ClientNotification::ClientNotification(ClientNotificationType type):
        mType(type)
{
    mPacket << type;
}

std::string ClientNotification::typeString(ClientNotificationType type)
{
    switch(type)
    {
        case ClientNotificationType::hello:
            return "hello";
        case ClientNotificationType::setNick:
            return "setNick";
        case ClientNotificationType::chat:
            return "chat";
        case ClientNotificationType::askCreaturePickUp:
            return "askCreaturePickUp";
        case ClientNotificationType::askCreatureDrop:
            return "askCreatureDrop";
        case ClientNotificationType::askMarkTile:
            return "askMarkTile";
        case ClientNotificationType::askBuildRoom:
            return "askBuildRoom";
        case ClientNotificationType::askBuildTrap:
            return "askBuildTrap";
        case ClientNotificationType::ackNewTurn:
            return "ackNewTurn";
        case ClientNotificationType::askCreatureInfos:
            return "askCreatureInfos";
        case ClientNotificationType::editorAskSaveMap:
            return "editorAskSaveMap";
        case ClientNotificationType::editorAskChangeTiles:
            return "editorAskChangeTiles";
        case ClientNotificationType::editorAskBuildRoom:
            return "editorAskBuildRoom";
        case ClientNotificationType::editorAskBuildTrap:
            return "editorAskBuildTrap";
        default:
            LogManager::getSingleton().logMessage("ERROR: Unknown enum for ClientNotificationType="
                + Ogre::StringConverter::toString(static_cast<int>(type)));
    }
    return "";
}

ODPacket& operator<<(ODPacket& os, const ClientNotification::ClientNotificationType& nt)
{
    os << static_cast<int32_t>(nt);
    return os;
}

ODPacket& operator>>(ODPacket& is, ClientNotification::ClientNotificationType& nt)
{
    int32_t tmp;
    is >> tmp;
    nt = static_cast<ClientNotification::ClientNotificationType>(tmp);
    return is;
}
