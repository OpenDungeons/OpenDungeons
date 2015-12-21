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

#include "creatureskill/CreatureSkillMeleeFight.h"

#include "creatureskill/CreatureSkillManager.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "entities/Weapon.h"
#include "gamemap/GameMap.h"
#include "spells/Spell.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <istream>

const std::string CreatureSkillMeleeFightName = "Melee";

namespace
{
class CreatureSkillMeleeFightFactory : public CreatureSkillFactory
{
    CreatureSkill* createCreatureSkill() const override
    { return new CreatureSkillMeleeFight; }

    const std::string& getCreatureSkillName() const override
    { return CreatureSkillMeleeFightName; }
};

// Register the factory
static CreatureSkillRegister reg(new CreatureSkillMeleeFightFactory);
}

const std::string& CreatureSkillMeleeFight::getSkillName() const
{
    return CreatureSkillMeleeFightName;
}

bool CreatureSkillMeleeFight::canBeUsedBy(const Creature* creature) const
{
    if(creature->getLevel() < mCreatureLevelMin)
        return false;

    return true;
}

bool CreatureSkillMeleeFight::tryUseFight(GameMap& gameMap, Creature* creature, float range,
        GameEntity* attackedObject, Tile* attackedTile, bool ko) const
{
    double level = static_cast<double>(creature->getLevel());
    double phyAtk = mPhyAtk + (level * mPhyAtkPerLvl);
    double magAtk = mMagAtk + (level * mMagAtkPerLvl);
    double eleAtk = mEleAtk + (level * mEleAtkPerLvl);
    if(creature->getWeaponL() != nullptr)
    {
        phyAtk +=creature->getWeaponL()->getPhysicalDamage();
        magAtk +=creature->getWeaponL()->getMagicalDamage();
        eleAtk +=creature->getWeaponL()->getElementDamage();
    }
    if(creature->getWeaponR() != nullptr)
    {
        phyAtk +=creature->getWeaponR()->getPhysicalDamage();
        magAtk +=creature->getWeaponR()->getMagicalDamage();
        eleAtk +=creature->getWeaponR()->getElementDamage();
    }

    double modifier = creature->getModifierStrength();
    if(modifier != 1.0)
    {
        phyAtk *= modifier;
        magAtk *= modifier;
        eleAtk *= modifier;
    }
    attackedObject->takeDamage(creature, 0.0, phyAtk, magAtk, eleAtk, attackedTile, ko);

    return true;
}

CreatureSkillMeleeFight* CreatureSkillMeleeFight::clone() const
{
    return new CreatureSkillMeleeFight(*this);
}

void CreatureSkillMeleeFight::getFormatString(std::string& format) const
{
    CreatureSkill::getFormatString(format);
    if(!format.empty())
        format += "\t";

    format += "LevelMin\tPhyAtk\tPhyAtkPerLvl\tMagAtk\tMagAtkPerLvl\tEleAtk\tEleAtkPerLvl";

}

void CreatureSkillMeleeFight::exportToStream(std::ostream& os) const
{
    CreatureSkill::exportToStream(os);
    os << "\t" << mCreatureLevelMin;
    os << "\t" << mPhyAtk;
    os << "\t" << mPhyAtkPerLvl;
    os << "\t" << mMagAtk;
    os << "\t" << mMagAtkPerLvl;
    os << "\t" << mEleAtk;
    os << "\t" << mEleAtkPerLvl;
}

bool CreatureSkillMeleeFight::importFromStream(std::istream& is)
{
    if(!CreatureSkill::importFromStream(is))
        return false;

    if(!(is >> mCreatureLevelMin))
        return false;
    if(!(is >> mPhyAtk))
        return false;
    if(!(is >> mPhyAtkPerLvl))
        return false;
    if(!(is >> mMagAtk))
        return false;
    if(!(is >> mMagAtkPerLvl))
        return false;
    if(!(is >> mEleAtk))
        return false;
    if(!(is >> mEleAtkPerLvl))
        return false;

    return true;
}

bool CreatureSkillMeleeFight::isEqual(const CreatureSkill& creatureSkill) const
{
    if(!CreatureSkill::isEqual(creatureSkill))
        return false;

    const CreatureSkillMeleeFight* skill = dynamic_cast<const CreatureSkillMeleeFight*>(&creatureSkill);
    if(skill == nullptr)
        return false;

    if(mCreatureLevelMin != skill->mCreatureLevelMin)
        return false;
    if(mPhyAtk != skill->mPhyAtk)
        return false;
    if(mPhyAtkPerLvl != skill->mPhyAtkPerLvl)
        return false;
    if(mMagAtk != skill->mMagAtk)
        return false;
    if(mMagAtkPerLvl != skill->mMagAtkPerLvl)
        return false;
    if(mEleAtk != skill->mEleAtk)
        return false;
    if(mEleAtkPerLvl != skill->mEleAtkPerLvl)
        return false;

    return true;
}
