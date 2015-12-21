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

#include "creatureskill/CreatureSkillStrengthSelf.h"

#include "creatureaction/CreatureAction.h"
#include "creatureeffect/CreatureEffectStrengthChange.h"
#include "creatureskill/CreatureSkillManager.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "gamemap/GameMap.h"
#include "spells/Spell.h"

#include <istream>

const std::string CreatureSkillStrengthSelfName = "StrengthSelf";

namespace
{
class CreatureSkillStrengthSelfFactory : public CreatureSkillFactory
{
    CreatureSkill* createCreatureSkill() const override
    { return new CreatureSkillStrengthSelf; }

    const std::string& getCreatureSkillName() const override
    { return CreatureSkillStrengthSelfName; }
};

// Register the factory
static CreatureSkillRegister reg(new CreatureSkillStrengthSelfFactory);
}

const std::string& CreatureSkillStrengthSelf::getSkillName() const
{
    return CreatureSkillStrengthSelfName;
}

bool CreatureSkillStrengthSelf::canBeUsedBy(const Creature* creature) const
{
    if(creature->getLevel() < mCreatureLevelMin)
        return false;

    return true;
}

bool CreatureSkillStrengthSelf::tryUseSupport(GameMap& gameMap, Creature* creature) const
{
    if(!creature->isAlive())
        return false;

    // To not waste the spell, we only cast it during a fight
    if(!creature->isActionInList(CreatureActionType::fight))
        return false;

    CreatureEffectStrengthChange* effect = new CreatureEffectStrengthChange(mEffectDuration, mEffectValue, "SpellCreatureStrength");
    creature->addCreatureEffect(effect);

    for(Tile* tile : creature->getCoveredTiles())
    {
        Spell::fireSpellSound(*tile, "Heal");
    }

    return true;
}

CreatureSkillStrengthSelf* CreatureSkillStrengthSelf::clone() const
{
    return new CreatureSkillStrengthSelf(*this);
}

void CreatureSkillStrengthSelf::getFormatString(std::string& format) const
{
    CreatureSkill::getFormatString(format);
    if(!format.empty())
        format += "\t";

    format += "LevelMin\tEffectDuration\tEffectValue";
}

void CreatureSkillStrengthSelf::exportToStream(std::ostream& os) const
{
    CreatureSkill::exportToStream(os);
    os << "\t" << mCreatureLevelMin;
    os << "\t" << mEffectDuration;
    os << "\t" << mEffectValue;
}

bool CreatureSkillStrengthSelf::importFromStream(std::istream& is)
{
    if(!CreatureSkill::importFromStream(is))
        return false;

    if(!(is >> mCreatureLevelMin))
        return false;
    if(!(is >> mEffectDuration))
        return false;
    if(!(is >> mEffectValue))
        return false;

    return true;
}

bool CreatureSkillStrengthSelf::isEqual(const CreatureSkill& creatureSkill) const
{
    if(!CreatureSkill::isEqual(creatureSkill))
        return false;

    const CreatureSkillStrengthSelf* skill = dynamic_cast<const CreatureSkillStrengthSelf*>(&creatureSkill);
    if(skill == nullptr)
        return false;

    if(mCreatureLevelMin != skill->mCreatureLevelMin)
        return false;
    if(mEffectDuration != skill->mEffectDuration)
        return false;
    if(mEffectValue != skill->mEffectValue)
        return false;

    return true;
}
