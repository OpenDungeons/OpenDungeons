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

#include "creaturemood/CreatureMoodCreature.h"

#include "entities/Creature.h"
#include "entities/GameEntity.h"

#include "gamemap/GameMap.h"

#include "utils/LogManager.h"

int32_t CreatureMoodCreature::computeMood(const Creature* creature) const
{
    std::vector<GameEntity*> alliedCreatures = creature->getGameMap()->getVisibleCreatures(creature->getVisibleTiles(),
        creature->getSeat(), false);
    int nbCreatures = 0;
    for(GameEntity* entity : alliedCreatures)
    {
        if(entity->getObjectType() != GameEntity::ObjectType::creature)
            continue;

        Creature* alliedCreature = static_cast<Creature*>(entity);
        if(alliedCreature->getDefinition() != mCreatureDefinition)
            continue;

        ++nbCreatures;
    }
    return nbCreatures * mMoodModifier;
}

void CreatureMoodCreature::init(GameMap* gameMap)
{
    mCreatureDefinition = gameMap->getClassDescription(mCreatureClass);
    OD_ASSERT_TRUE_MSG(mCreatureDefinition != nullptr, "Unknown creature class=" + mCreatureClass);
}
