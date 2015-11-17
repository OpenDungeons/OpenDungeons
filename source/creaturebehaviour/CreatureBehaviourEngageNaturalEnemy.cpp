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

#include "creaturebehaviour/CreatureBehaviourEngageNaturalEnemy.h"

#include "creatureaction/CreatureAction.h"
#include "creaturebehaviour/CreatureBehaviourManager.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "creaturemood/CreatureMood.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "utils/Random.h"

const std::string CreatureBehaviourEngageNaturalEnemy::mNameCreatureBehaviourEngageNaturalEnemy = "EngageNaturalEnemy";

namespace
{
class CreatureBehaviourFactoryEngageNaturalEnemy : public CreatureBehaviourFactory
{
    CreatureBehaviour* createCreatureBehaviour() const override
    { return new CreatureBehaviourEngageNaturalEnemy; }

    const std::string& getCreatureBehaviourName() const override
    {
        return CreatureBehaviourEngageNaturalEnemy::mNameCreatureBehaviourEngageNaturalEnemy;
    }
};

// Register the factory
static CreatureBehaviourRegister reg(new CreatureBehaviourFactoryEngageNaturalEnemy);
}

CreatureBehaviourEngageNaturalEnemy::CreatureBehaviourEngageNaturalEnemy(const CreatureBehaviourEngageNaturalEnemy& behaviour)
{
    for(const std::string& str : behaviour.mNaturalEnemyClasses)
    {
        mNaturalEnemyClasses.push_back(str);
    }
}

CreatureBehaviour* CreatureBehaviourEngageNaturalEnemy::clone() const
{
    return new CreatureBehaviourEngageNaturalEnemy(*this);
}

bool CreatureBehaviourEngageNaturalEnemy::processBehaviour(Creature& creature) const
{
    // The creature should attack natural enemies if at least angry
    if(creature.getMoodValue() < CreatureMoodLevel::Upset)
        return true;

    if(Random::Int(0, 100) < 80)
        return true;

    // If the creature is already fighting, it should not engage another creature
    if(creature.isActionInList(CreatureActionType::fight))
        return true;

    // We look for a natural enemy
    std::vector<Creature*> alliedNaturalEnemies;
    for(GameEntity* entity : creature.getReachableAlliedObjects())
    {
        if(entity->getObjectType() != GameEntityType::creature)
            continue;

        Creature* alliedCreature = static_cast<Creature*>(entity);
        // Check if the given creature is a natural enemy
        for(const std::string& enemyClass : mNaturalEnemyClasses)
        {
            if(alliedCreature->getDefinition()->getClassName().compare(enemyClass) != 0)
                continue;

            alliedNaturalEnemies.push_back(alliedCreature);
        }
    }

    if(alliedNaturalEnemies.empty())
        return true;

    uint32_t index = Random::Uint(0, alliedNaturalEnemies.size() - 1);
    Creature& target = *alliedNaturalEnemies.at(index);
    creature.engageAlliedNaturalEnemy(target);
    target.engageAlliedNaturalEnemy(creature);

    return false;
}

void CreatureBehaviourEngageNaturalEnemy::getFormatString(std::string& format) const
{
    if(!format.empty())
        format += "\t";

    format += "nbEnemy\tN*EnemyClassName";
}

bool CreatureBehaviourEngageNaturalEnemy::isEqual(const CreatureBehaviour& creatureBehaviour) const
{
    if(!CreatureBehaviour::isEqual(creatureBehaviour))
        return false;

    const CreatureBehaviourEngageNaturalEnemy* cb = dynamic_cast<const CreatureBehaviourEngageNaturalEnemy*>(&creatureBehaviour);
    if(cb == nullptr)
        return false;

    if(mNaturalEnemyClasses.size() != cb->mNaturalEnemyClasses.size())
        return false;

    for(uint32_t i = 0; i < mNaturalEnemyClasses.size(); ++i)
    {
        if(mNaturalEnemyClasses[i].compare(cb->mNaturalEnemyClasses[i]) != 0)
            return false;
    }

    return true;
}

void CreatureBehaviourEngageNaturalEnemy::exportToStream(std::ostream& os) const
{
    CreatureBehaviour::exportToStream(os);
    uint32_t nb = mNaturalEnemyClasses.size();
    os << "\t" << nb;
    for(const std::string& str : mNaturalEnemyClasses)
    {
        os << "\t" << str;
    }
}

bool CreatureBehaviourEngageNaturalEnemy::importFromStream(std::istream& is)
{
    if(!CreatureBehaviour::importFromStream(is))
        return false;

    uint32_t nb;
    if(!(is >> nb))
        return false;

    std::string str;
    while(nb > 0)
    {
        --nb;
        if(!(is >> str))
            return false;

        mNaturalEnemyClasses.push_back(str);
    }

    return true;
}
