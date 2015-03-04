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

#include "ClientNotification.h"
#include "utils/LogManager.h"

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
        case ClientNotificationType::readyForSeatConfiguration:
            return "readyForSeatConfiguration";
        case ClientNotificationType::seatConfigurationSet:
            return "seatConfigurationSet";
        case ClientNotificationType::seatConfigurationRefresh:
            return "seatConfigurationRefresh";
        case ClientNotificationType::askEntityPickUp:
            return "askEntityPickUp";
        case ClientNotificationType::askHandDrop:
            return "askHandDrop";
        case ClientNotificationType::askMarkTile:
            return "askMarkTile";
        case ClientNotificationType::askBuildRoom:
            return "askBuildRoom";
        case ClientNotificationType::askSellRoomTiles:
            return "askSellRoomTiles";
        case ClientNotificationType::editorAskDestroyRoomTiles:
            return "editorAskDestroyRoomTiles";
        case ClientNotificationType::askBuildTrap:
            return "askBuildTrap";
        case ClientNotificationType::askSellTrapTiles:
            return "askSellTrapTiles";
        case ClientNotificationType::editorAskDestroyTrapTiles:
            return "editorAskDestroyTrapTiles";
        case ClientNotificationType::ackNewTurn:
            return "ackNewTurn";
        case ClientNotificationType::askCreatureInfos:
            return "askCreatureInfos";
        case ClientNotificationType::askPickupWorker:
            return "askPickupWorker";
        case ClientNotificationType::askPickupFighter:
            return "askPickupFighter";
        case ClientNotificationType::askCastSpell:
            return "askCastSpell";
        case ClientNotificationType::askSetResearchTree:
            return "askSetResearchTree";
        case ClientNotificationType::askSaveMap:
            return "askSaveMap";
        case ClientNotificationType::editorAskChangeTiles:
            return "editorAskChangeTiles";
        case ClientNotificationType::editorAskBuildRoom:
            return "editorAskBuildRoom";
        case ClientNotificationType::editorAskBuildTrap:
            return "editorAskBuildTrap";
        case ClientNotificationType::editorCreateWorker:
            return "editorCreateWorker";
        case ClientNotificationType::editorCreateFighter:
            return "editorCreateFighter";
        case ClientNotificationType::editorAskCreateMapLight:
            return "editorAskCreateMapLight";
        default:
            LogManager::getSingleton().logMessage("ERROR: Unknown enum for ClientNotificationType="
                + Ogre::StringConverter::toString(static_cast<int>(type)));
    }
    return "";
}

ODPacket& operator<<(ODPacket& os, const ClientNotificationType& nt)
{
    os << static_cast<int32_t>(nt);
    return os;
}

ODPacket& operator>>(ODPacket& is, ClientNotificationType& nt)
{
    int32_t tmp;
    is >> tmp;
    nt = static_cast<ClientNotificationType>(tmp);
    return is;
}
