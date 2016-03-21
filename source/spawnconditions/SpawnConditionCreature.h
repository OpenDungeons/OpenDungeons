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

#ifndef SPAWNCONDITIONCREATURE_H
#define SPAWNCONDITIONCREATURE_H

#include "spawnconditions/SpawnCondition.h"

class CreatureDefinition;

class SpawnConditionCreature : public SpawnCondition
{
public:
    // Constructors
    SpawnConditionCreature(const CreatureDefinition* creatureDefinition, int32_t nbCreatureMin, int32_t pointsPerAdditionalCreature) :
        mCreatureDefinition(creatureDefinition),
        mNbCreatureMin(nbCreatureMin),
        mPointsPerAdditionalCreature(pointsPerAdditionalCreature)
    {}

    virtual ~SpawnConditionCreature() {}

    //! \brief Checks if this spawning condition is met for the given gameMap/Seat. Returns true if the conditions are met and
    //! false otherwise. If true, computedPoints will be set to the additional points (can be < 0).
    virtual bool computePointsForSeat(const GameMap& gameMap, const Seat& seat, int32_t& computedPoints) const;

private:
    const CreatureDefinition* mCreatureDefinition;
    int32_t mNbCreatureMin;
    int32_t mPointsPerAdditionalCreature;
};

#endif // SPAWNCONDITIONCREATURE_H
