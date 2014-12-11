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

#include "network/ServerNotification.h"
#include "utils/LogManager.h"

#include <OgreStringConverter.h>

ServerNotification::ServerNotification(ServerNotificationType type,
    Player* concernedPlayer)
{
    mType = type;
    mConcernedPlayer = concernedPlayer;
    mPacket << type;
}

std::string ServerNotification::typeString(ServerNotificationType type)
{
    switch(type)
    {
        case ServerNotificationType::pickNick:
            return "pickNick";
        case ServerNotificationType::addPlayers:
            return "addPlayers";
        case ServerNotificationType::removePlayers:
            return "removePlayers";
        case ServerNotificationType::startGameMode:
            return "startGameMode";
        case ServerNotificationType::newMap:
            return "newMap";
        case ServerNotificationType::addTile:
            return "addTile";
        case ServerNotificationType::addMapLight:
            return "addMapLight";
        case ServerNotificationType::removeMapLight:
            return "removeMapLight";
        case ServerNotificationType::addClass:
            return "addClass";
        case ServerNotificationType::clientAccepted:
            return "clientAccepted";
        case ServerNotificationType::clientRejected:
            return "clientRejected";
        case ServerNotificationType::seatConfigurationRefresh:
            return "seatConfigurationRefresh";
        case ServerNotificationType::chat:
            return "chat";
        case ServerNotificationType::chatServer:
            return "chatServer";
        case ServerNotificationType::playerWon:
            return "playerWon";
        case ServerNotificationType::playerLost:
            return "playerLost";
        case ServerNotificationType::markTiles:
            return "markTiles";
        case ServerNotificationType::turnStarted:
            return "turnStarted";
        case ServerNotificationType::setTurnsPerSecond:
            return "setTurnsPerSecond";
        case ServerNotificationType::buildRoom:
            return "buildRoom";
        case ServerNotificationType::removeRoomTile:
            return "removeRoomTile";
        case ServerNotificationType::buildTrap:
            return "buildTrap";
        case ServerNotificationType::removeTrapTile:
            return "removeTrapTile";
        case ServerNotificationType::animatedObjectAddDestination:
            return "animatedObjectAddDestination";
        case ServerNotificationType::animatedObjectClearDestinations:
            return "animatedObjectClearDestinations";
        case ServerNotificationType::setObjectAnimationState:
            return "setObjectAnimationState";
        case ServerNotificationType::setMoveSpeed:
            return "setMoveSpeed";
        case ServerNotificationType::entityPickedUp:
            return "entityPickedUp";
        case ServerNotificationType::entityDropped:
            return "entityDropped";
        case ServerNotificationType::addCreature:
            return "addCreature";
        case ServerNotificationType::removeCreature:
            return "removeCreature";
        case ServerNotificationType::creatureRefresh:
            return "creatureRefresh";
        case ServerNotificationType::playerFighting:
            return "playerFighting";
        case ServerNotificationType::playerNoMoreFighting:
            return "playerNoMoreFighting";
        case ServerNotificationType::refreshPlayerSeat:
            return "refreshPlayerSeat";
        case ServerNotificationType::addRenderedMovableEntity:
            return "addRenderedMovableEntity";
        case ServerNotificationType::removeRenderedMovableEntity:
            return "removeRenderedMovableEntity";
        case setEntityOpacity:
            return "setEntityOpacity";
        case ServerNotificationType::playSpatialSound:
            return "playSpatialSound";
        case ServerNotificationType::notifyCreatureInfo:
            return "notifyCreatureInfo";
        case ServerNotificationType::refreshCreatureVisDebug:
            return "refreshCreatureVisDebug";
        case ServerNotificationType::playCreatureSound:
            return "playCreatureSound";
        case ServerNotificationType::refreshTiles:
            return "refreshTiles";
        case ServerNotificationType::exit:
            return "exit";
        default:
            LogManager::getSingleton().logMessage("ERROR: Unknown enum for ServerNotificationType="
                + Ogre::StringConverter::toString(static_cast<int>(type)));
    }
    return "";
}

ODPacket& operator<<(ODPacket& os, const ServerNotification::ServerNotificationType& nt)
{
    os << static_cast<int32_t>(nt);
    return os;
}

ODPacket& operator>>(ODPacket& is, ServerNotification::ServerNotificationType& nt)
{
    int32_t tmp;
    is >> tmp;
    nt = static_cast<ServerNotification::ServerNotificationType>(tmp);
    return is;
}
