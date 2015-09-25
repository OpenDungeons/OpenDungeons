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

#include "entities/Creature.h"
#include "entities/Tile.h"
#include "entities/Weapon.h"
#include "gamemap/GameMap.h"
#include "sound/SoundEffectsManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <istream>

bool CreatureSkillMeleeFight::canBeUsedBy(const Creature* creature) const
{
    if(creature->getLevel() < mCreatureLevelMin)
        return false;

    return true;
}

bool CreatureSkillMeleeFight::tryUseFight(GameMap& gameMap, Creature* creature, float range,
        GameEntity* attackedObject, Tile* attackedTile) const
{
    if(attackedObject->getObjectType() != GameEntityType::creature)
    {
        OD_LOG_ERR("creature=" + creature->getName() + ", attackedObject=" + attackedObject->getName() + ", attackedTile=" + Tile::displayAsString(attackedTile));
        return false;
    }

    double level = static_cast<double>(creature->getLevel());
    double phyAtk = mPhyAtk + (level * mPhyAtkPerLvl);
    double magAtk = mMagAtk + (level * mMagAtkPerLvl);
    if(creature->getWeaponL() != nullptr)
    {
        phyAtk +=creature->getWeaponL()->getPhysicalDamage();
        magAtk +=creature->getWeaponL()->getMagicalDamage();
    }
    if(creature->getWeaponR() != nullptr)
    {
        phyAtk +=creature->getWeaponR()->getPhysicalDamage();
        magAtk +=creature->getWeaponR()->getMagicalDamage();
    }
    attackedObject->takeDamage(creature, phyAtk, magAtk, attackedTile, false, false);

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

    format += "LevelMin\tPhyAtk\tPhyAtkPerLvl\tMagAtk\tMagAtkPerLvl";

}

void CreatureSkillMeleeFight::exportToStream(std::ostream& os) const
{
    CreatureSkill::exportToStream(os);
    os << "\t" << mCreatureLevelMin;
    os << "\t" << mPhyAtk;
    os << "\t" << mPhyAtkPerLvl;
    os << "\t" << mMagAtk;
    os << "\t" << mMagAtkPerLvl;
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

    return true;
}
