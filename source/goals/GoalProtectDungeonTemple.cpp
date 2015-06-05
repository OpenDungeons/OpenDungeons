/*
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

#include "goals/GoalProtectDungeonTemple.h"

#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "rooms/RoomType.h"

#include <vector>
#include <iostream>

GoalProtectDungeonTemple::GoalProtectDungeonTemple(const std::string& nName, const std::string& nArguments) :
    Goal(nName, nArguments)
{
}

bool GoalProtectDungeonTemple::isMet(const Seat& s, const GameMap& gameMap)
{
    return (gameMap.numRoomsByTypeAndSeat(RoomType::dungeonTemple, &s) > 0);
}

bool GoalProtectDungeonTemple::isUnmet(const Seat&, const GameMap&)
{
    return false;
}

bool GoalProtectDungeonTemple::isFailed(const Seat& s, const GameMap& gameMap)
{
    return !isMet(s, gameMap);
}

std::string GoalProtectDungeonTemple::getDescription(const Seat&)
{
    return "Protect your dungeon temple.";
}

std::string GoalProtectDungeonTemple::getSuccessMessage(const Seat&)
{
    return "Your dungeon temple is intact";
}

std::string GoalProtectDungeonTemple::getFailedMessage(const Seat&)
{
    return "Your dungeon temple has been destroyed";
}
