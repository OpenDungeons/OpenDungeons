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

#include "game/Seat.h"

#include "spawnconditions/SpawnConditionGold.h"

bool SpawnConditionGold::computePointsForSeat(const GameMap&, const Seat& seat, int32_t& computedPoints) const
{
    if(seat.getGold() < mNbGoldMin)
        return false;

    int diffGold = (seat.getGold() - mNbGoldMin) / 100;

    computedPoints = diffGold * mPointsPerAdditional100Gold;
    return true;
}
