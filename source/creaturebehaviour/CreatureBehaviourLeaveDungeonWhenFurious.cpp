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

#include "creaturebehaviour/CreatureBehaviourLeaveDungeonWhenFurious.h"

#include "creaturebehaviour/CreatureBehaviourManager.h"
#include "entities/Creature.h"
#include "creaturemood/CreatureMood.h"

const std::string CreatureBehaviourLeaveDungeonWhenFurious::mNameCreatureBehaviourLeaveDungeonWhenFurious = "LeaveDungeonWhenFurious";

namespace
{
class CreatureBehaviourLeaveDungeonWhenFuriousFactory : public CreatureBehaviourFactory
{
    CreatureBehaviour* createCreatureBehaviour() const override
    { return new CreatureBehaviourLeaveDungeonWhenFurious; }

    const std::string& getCreatureBehaviourName() const override
    {
        return CreatureBehaviourLeaveDungeonWhenFurious::mNameCreatureBehaviourLeaveDungeonWhenFurious;
    }
};

// Register the factory
static CreatureBehaviourRegister reg(new CreatureBehaviourLeaveDungeonWhenFuriousFactory);
}

CreatureBehaviour* CreatureBehaviourLeaveDungeonWhenFurious::clone() const
{
    return new CreatureBehaviourLeaveDungeonWhenFurious;
}

bool CreatureBehaviourLeaveDungeonWhenFurious::processBehaviour(Creature& creature) const
{
    // The creature should try to leave if furious
    if(creature.getMoodValue() < CreatureMoodLevel::Furious)
        return true;

    if(!creature.isActionInList(CreatureActionType::leaveDungeon))
    {
        creature.clearDestinations(EntityAnimation::idle_anim, true);
        creature.clearActionQueue();
        creature.stopJob();
        creature.stopEating();
        creature.pushAction(CreatureActionType::leaveDungeon, false, true);
    }

    // If the creature is furious, it does nothing else
    return false;
}
