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

#include "gamemap/GameMap.h"

#include "rooms/Room.h"

#include "spawnconditions/SpawnConditionRoom.h"

bool SpawnConditionRoom::computePointsForSeat(const GameMap& gameMap, const Seat& seat, int32_t& computedPoints) const
{
    int32_t nbActiveSpots = 0;
    std::vector<const Room*> rooms = gameMap.getRoomsByTypeAndSeat(mRoomType, &seat);
    for(const Room* room : rooms)
    {
        nbActiveSpots += room->getNumActiveSpots();
    }
    if(nbActiveSpots < mNbActiveSpotsMin)
        return false;

    computedPoints = (nbActiveSpots - mNbActiveSpotsMin) * mPointsPerAdditionalActiveSpots;
    return true;
}
