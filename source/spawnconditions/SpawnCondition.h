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

#ifndef SPAWNCONDITION_H
#define SPAWNCONDITION_H

#include <cstdint>
#include <istream>
#include <vector>

class GameMap;
class Seat;

class SpawnCondition
{
public:
    // Constructors
    SpawnCondition()
    {}

    virtual ~SpawnCondition()
    {}

    static SpawnCondition* load(std::istream& defFile);

    //! \brief Checks if this spawning condition is met for the given gameMap/Seat. Returns true if the conditions are met and
    //! false otherwise. If true, computedPoints will be set to the additional points (can be < 0).
    virtual bool computePointsForSeat(GameMap* gameMap, Seat* seat, int32_t& computedPoints) const = 0;

    static const std::vector<const SpawnCondition*> EMPTY_SPAWNCONDITIONS;
};

#endif // SPAWNCONDITION_H
