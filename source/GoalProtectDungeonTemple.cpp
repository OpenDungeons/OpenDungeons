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

#include "GoalProtectDungeonTemple.h"

#include "GameMap.h"
#include "Room.h"
#include "Seat.h"

#include <vector>
#include <iostream>

GoalProtectDungeonTemple::GoalProtectDungeonTemple(const std::string& nName, const std::string& nArguments, GameMap* gameMap) :
    Goal(nName, nArguments, gameMap)
{
    std::cout << "\nAdding goal " << getName();
}

bool GoalProtectDungeonTemple::isMet(Seat *s)
{
    return (mGameMap->numRoomsByTypeAndColor(Room::dungeonTemple, s->getColor()) > 0);
}

bool GoalProtectDungeonTemple::isUnmet(Seat *s)
{
    return false;
}

bool GoalProtectDungeonTemple::isFailed(Seat *s)
{
    return !isMet(s);
}

std::string GoalProtectDungeonTemple::getDescription(Seat *s)
{
    return "Protect your dungeon temple.";
}

std::string GoalProtectDungeonTemple::getSuccessMessage(Seat *s)
{
    return "Your dungeon temple is intact";
}

std::string GoalProtectDungeonTemple::getFailedMessage(Seat *s)
{
    return "Your dungeon temple has been destroyed";
}
