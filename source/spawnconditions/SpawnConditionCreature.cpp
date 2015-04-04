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

#include "entities/Creature.h"

#include "gamemap/GameMap.h"

#include "spawnconditions/SpawnConditionCreature.h"

bool SpawnConditionCreature::computePointsForSeat(const GameMap& gameMap, const Seat& seat, int32_t& computedPoints) const
{
    int32_t nbCreatures = 0;
    std::vector<Creature*> creatures = gameMap.getCreaturesBySeat(&seat);
    for(Creature* creature : creatures)
    {
        if(creature->getDefinition() == mCreatureDefinition)
            ++nbCreatures;
    }
    if(nbCreatures < mNbCreatureMin)
        return false;

    computedPoints = (nbCreatures - mNbCreatureMin) * mPointsPerAdditionalCreature;
    return true;
}
