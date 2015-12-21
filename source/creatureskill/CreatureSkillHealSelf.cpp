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

#include "creatureskill/CreatureSkillHealSelf.h"

#include "creatureeffect/CreatureEffectHeal.h"
#include "creatureskill/CreatureSkillManager.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "gamemap/GameMap.h"
#include "spells/Spell.h"

#include <istream>

const std::string CreatureSkillHealSelfName = "HealSelf";

namespace
{
class CreatureSkillHealSelfFactory : public CreatureSkillFactory
{
    CreatureSkill* createCreatureSkill() const override
    { return new CreatureSkillHealSelf; }

    const std::string& getCreatureSkillName() const override
    { return CreatureSkillHealSelfName; }
};

// Register the factory
static CreatureSkillRegister reg(new CreatureSkillHealSelfFactory);
}

const std::string& CreatureSkillHealSelf::getSkillName() const
{
    return CreatureSkillHealSelfName;
}

bool CreatureSkillHealSelf::canBeUsedBy(const Creature* creature) const
{
    if(creature->getLevel() < mCreatureLevelMin)
        return false;

    return true;
}

bool CreatureSkillHealSelf::tryUseSupport(GameMap& gameMap, Creature* creature) const
{
    if(!creature->isAlive())
        return false;

    if(!creature->isHurt())
        return false;

    CreatureEffectHeal* effect = new CreatureEffectHeal(mEffectDuration, mEffectValue, "SpellCreatureHeal");
    creature->addCreatureEffect(effect);

    for(Tile* tile : creature->getCoveredTiles())
    {
        Spell::fireSpellSound(*tile, "Heal");
    }

    return true;
}

CreatureSkillHealSelf* CreatureSkillHealSelf::clone() const
{
    return new CreatureSkillHealSelf(*this);
}

void CreatureSkillHealSelf::getFormatString(std::string& format) const
{
    CreatureSkill::getFormatString(format);
    if(!format.empty())
        format += "\t";

    format += "LevelMin\tEffectDuration\tEffectValue";
}

void CreatureSkillHealSelf::exportToStream(std::ostream& os) const
{
    CreatureSkill::exportToStream(os);
    os << "\t" << mCreatureLevelMin;
    os << "\t" << mEffectDuration;
    os << "\t" << mEffectValue;
}

bool CreatureSkillHealSelf::importFromStream(std::istream& is)
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

bool CreatureSkillHealSelf::isEqual(const CreatureSkill& creatureSkill) const
{
    if(!CreatureSkill::isEqual(creatureSkill))
        return false;

    const CreatureSkillHealSelf* skill = dynamic_cast<const CreatureSkillHealSelf*>(&creatureSkill);
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
