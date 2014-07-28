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

#include "ServerNotification.h"
#include "LogManager.h"

#include <OgreStringConverter.h>

ServerNotification::ServerNotification(ServerNotificationType type,
    Player* concernedPlayer)
{
    mType = type;
    mConcernedPlayer = concernedPlayer;
    packet << typeString(type);
}

std::string ServerNotification::typeString(ServerNotificationType type)
{
    switch(type)
    {
        case ServerNotificationType::pickNick:
            return "pickNick";
        case ServerNotificationType::yourSeat:
            return "yourSeat";
        case ServerNotificationType::addPlayer:
            return "addPlayer";
        case ServerNotificationType::newMap:
            return "newMap";
        case ServerNotificationType::turnsPerSecond:
            return "turnsPerSecond";
        case ServerNotificationType::addTile:
            return "addTile";
        case ServerNotificationType::addMapLight:
            return "addMapLight";
        case ServerNotificationType::removeMapLight:
            return "removeMapLight";
        case ServerNotificationType::addClass:
            return "addClass";
        case ServerNotificationType::chat:
            return "chat";
        case ServerNotificationType::markTiles:
            return "markTiles";
        case ServerNotificationType::turnStarted:
            return "turnStarted";
        case ServerNotificationType::setTurnsPerSecond:
            return "setTurnsPerSecond";
        case ServerNotificationType::tileFullnessChange:
            return "tileFullnessChange";
        case ServerNotificationType::buildRoom:
            return "buildRoom";
        case ServerNotificationType::buildTrap:
            return "buildTrap";
        case ServerNotificationType::animatedObjectAddDestination:
            return "animatedObjectAddDestination";
        case ServerNotificationType::animatedObjectClearDestinations:
            return "animatedObjectClearDestinations";
        case ServerNotificationType::setObjectAnimationState:
            return "setObjectAnimationState";
        case ServerNotificationType::creaturePickedUp:
            return "creaturePickedUp";
        case ServerNotificationType::creatureDropped:
            return "creatureDropped";
        case ServerNotificationType::addCreature:
            return "addCreature";
        case ServerNotificationType::tileClaimed:
            return "tileClaimed";
        case ServerNotificationType::refreshPlayerSeat:
            return "refreshPlayerSeat";
        case ServerNotificationType::addCreatureBed:
            return "addCreatureBed";
        case ServerNotificationType::removeCreatureBed:
            return "removeCreatureBed";
        case ServerNotificationType::createTreasuryIndicator:
            return "createTreasuryIndicator";
        case ServerNotificationType::destroyTreasuryIndicator:
            return "destroyTreasuryIndicator";
        case ServerNotificationType::exit:
            return "exit";
        default:
            LogManager::getSingleton().logMessage("ERROR: Unknown enum for ServerNotificationType="
                + Ogre::StringConverter::toString(static_cast<int>(type)));
    }
    return "";
}
