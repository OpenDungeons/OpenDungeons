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

#include "creaturebehaviour/CreatureBehaviourAttackEnemy.h"

#include "creaturebehaviour/CreatureBehaviourManager.h"
#include "creaturemood/CreatureMood.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "utils/Random.h"

const std::string CreatureBehaviourAttackEnemy::mNameCreatureBehaviourAttackEnemy = "AttackEnemy";

namespace
{
class CreatureBehaviourAttackEnemyFactory : public CreatureBehaviourFactory
{
    CreatureBehaviour* createCreatureBehaviour() const override
    { return new CreatureBehaviourAttackEnemy; }

    const std::string& getCreatureBehaviourName() const override
    {
        return CreatureBehaviourAttackEnemy::mNameCreatureBehaviourAttackEnemy;
    }
};

// Register the factory
static CreatureBehaviourRegister reg(new CreatureBehaviourAttackEnemyFactory);
}

CreatureBehaviour* CreatureBehaviourAttackEnemy::clone() const
{
    return new CreatureBehaviourAttackEnemy;
}

bool CreatureBehaviourAttackEnemy::processBehaviour(Creature& creature) const
{
    // Check if we are already fighting or fleeing
    if(creature.isActionInList(CreatureActionType::fight) || creature.isActionInList(CreatureActionType::flee))
        return false;

    if (creature.getVisibleEnemyObjects().empty())
        return true;

    // Unhappy creatures might flee instead of engaging enemies
    switch(creature.getMoodValue())
    {
        case CreatureMoodLevel::Angry:
        case CreatureMoodLevel::Furious:
        {
            if(Random::Int(0,100) > 80)
            {
                creature.clearDestinations(EntityAnimation::idle_anim, true);
                creature.clearActionQueue();
                creature.pushAction(CreatureActionType::flee, false, true);
                return false;
            }
            break;
        }
        default:
            break;
    }

    // If we are not already fighting with a creature then start doing so.
    creature.clearDestinations(EntityAnimation::idle_anim, true);
    creature.clearActionQueue();
    creature.pushAction(CreatureActionType::fight, false, true);

    return false;
}
