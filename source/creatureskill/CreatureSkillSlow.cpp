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

#include "creatureskill/CreatureSkillSlow.h"

#include "creatureeffect/CreatureEffectSpeedChange.h"
#include "creatureskill/CreatureSkillManager.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "gamemap/GameMap.h"
#include "sound/SoundEffectsManager.h"
#include "utils/LogManager.h"

#include <istream>

const std::string CreatureSkillSlowName = "Slow";

namespace
{
class CreatureSkillSlowFactory : public CreatureSkillFactory
{
    CreatureSkill* createCreatureSkill() const override
    { return new CreatureSkillSlow; }

    const std::string& getCreatureSkillName() const override
    { return CreatureSkillSlowName; }
};

// Register the factory
static CreatureSkillRegister reg(new CreatureSkillSlowFactory);
}

const std::string& CreatureSkillSlow::getSkillName() const
{
    return CreatureSkillSlowName;
}

double CreatureSkillSlow::getRangeMax(const Creature* creature, GameEntity* entityAttack) const
{
    // Slow can be cast on creatures only
    if(entityAttack->getObjectType() != GameEntityType::creature)
        return 0.0;

    return mMaxRange;
}

bool CreatureSkillSlow::canBeUsedBy(const Creature* creature) const
{
    if(creature->getLevel() < mCreatureLevelMin)
        return false;

    return true;
}

bool CreatureSkillSlow::tryUseFight(GameMap& gameMap, Creature* creature, float range,
        GameEntity* attackedObject, Tile* attackedTile, bool ko) const
{
    if(attackedObject->getObjectType() != GameEntityType::creature)
    {
        OD_LOG_ERR("creature=" + creature->getName() + ", attackedObject=" + attackedObject->getName() + ", attackedTile=" + Tile::displayAsString(attackedTile));
        return false;
    }

    Creature* attackedCreature = static_cast<Creature*>(attackedObject);
    CreatureEffectSpeedChange* effect = new CreatureEffectSpeedChange(mEffectDuration, mEffectValue, "SpellCreatureSlow");
    attackedCreature->addCreatureEffect(effect);

    return true;
}

CreatureSkillSlow* CreatureSkillSlow::clone() const
{
    return new CreatureSkillSlow(*this);
}

void CreatureSkillSlow::getFormatString(std::string& format) const
{
    CreatureSkill::getFormatString(format);
    if(!format.empty())
        format += "\t";

    format += "RangeMax\tLevelMin\tEffectDuration\tEffectValue";

}

void CreatureSkillSlow::exportToStream(std::ostream& os) const
{
    CreatureSkill::exportToStream(os);
    os << "\t" << mMaxRange;
    os << "\t" << mCreatureLevelMin;
    os << "\t" << mEffectDuration;
    os << "\t" << mEffectValue;
}

bool CreatureSkillSlow::importFromStream(std::istream& is)
{
    if(!CreatureSkill::importFromStream(is))
        return false;

    if(!(is >> mMaxRange))
        return false;
    if(!(is >> mCreatureLevelMin))
        return false;
    if(!(is >> mEffectDuration))
        return false;
    if(!(is >> mEffectValue))
        return false;

    return true;
}

bool CreatureSkillSlow::isEqual(const CreatureSkill& creatureSkill) const
{
    if(!CreatureSkill::isEqual(creatureSkill))
        return false;

    const CreatureSkillSlow* skill = dynamic_cast<const CreatureSkillSlow*>(&creatureSkill);
    if(skill == nullptr)
        return false;

    if(mMaxRange != skill->mMaxRange)
        return false;
    if(mCreatureLevelMin != skill->mCreatureLevelMin)
        return false;
    if(mEffectDuration != skill->mEffectDuration)
        return false;
    if(mEffectValue != skill->mEffectValue)
        return false;

    return true;
}
