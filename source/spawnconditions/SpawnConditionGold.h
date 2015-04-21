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

#ifndef SPAWNCONDITIONGOLD_H
#define SPAWNCONDITIONGOLD_H

#include "spawnconditions/SpawnCondition.h"

class SpawnConditionGold : public SpawnCondition
{
public:
    // Constructors
        // Constructors
    SpawnConditionGold(int32_t nbGoldMin, int32_t pointsPerAdditional100Gold) :
        mNbGoldMin(nbGoldMin),
        mPointsPerAdditional100Gold(pointsPerAdditional100Gold)
    {}

    virtual ~SpawnConditionGold() {}

    //! \brief Checks if this spawning condition is met for the given gameMap/Seat. Returns true if the conditions are met and
    //! false otherwise. If true, computedPoints will be set to the additional points (can be < 0).
    virtual bool computePointsForSeat(const GameMap&, const Seat& seat, int32_t& computedPoints) const;

private:
    int32_t mNbGoldMin;
    int32_t mPointsPerAdditional100Gold;
};

#endif // SPAWNCONDITIONGOLD_H
