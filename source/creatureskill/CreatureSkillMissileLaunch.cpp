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

#include "creatureskill/CreatureSkillMissileLaunch.h"

#include "creatureskill/CreatureSkillManager.h"
#include "entities/Creature.h"
#include "entities/MissileOneHit.h"
#include "entities/Tile.h"
#include "entities/Weapon.h"
#include "gamemap/GameMap.h"
#include "sound/SoundEffectsManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <istream>

const Ogre::Real CANNON_MISSILE_HEIGHT = 0.3;
const std::string CreatureSkillMissileLaunchName = "MissileLaunch";

namespace
{
class CreatureSkillMissileLaunchFactory : public CreatureSkillFactory
{
    CreatureSkill* createCreatureSkill() const override
    { return new CreatureSkillMissileLaunch; }

    const std::string& getCreatureSkillName() const override
    { return CreatureSkillMissileLaunchName; }
};

// Register the factory
static CreatureSkillRegister reg(new CreatureSkillMissileLaunchFactory);
}

const std::string& CreatureSkillMissileLaunch::getSkillName() const
{
    return CreatureSkillMissileLaunchName;
}

double CreatureSkillMissileLaunch::getRangeMax(const Creature* creature, GameEntity* entityAttack) const
{
    double level = static_cast<double>(creature->getLevel());
    return mRangeMax + (level * mRangePerLvl);
}

bool CreatureSkillMissileLaunch::canBeUsedBy(const Creature* creature) const
{
    if(creature->getLevel() < mCreatureLevelMin)
        return false;

    return true;
}

bool CreatureSkillMissileLaunch::tryUseFight(GameMap& gameMap, Creature* creature, float range,
        GameEntity* attackedObject, Tile* attackedTile) const
{
    Tile* creatureTile = creature->getPositionTile();
    if(creatureTile == nullptr)
    {
        OD_LOG_ERR("creature=" + creature->getName() + ", pos=" + Helper::toString(creature->getPosition()));
        return false;
    }

    Ogre::Vector3 position;
    position.x = static_cast<Ogre::Real>(creatureTile->getX());
    position.y = static_cast<Ogre::Real>(creatureTile->getY());
    position.z = CANNON_MISSILE_HEIGHT;
    Ogre::Vector3 missileDirection(static_cast<Ogre::Real>(attackedTile->getX()),
        static_cast<Ogre::Real>(attackedTile->getY()), CANNON_MISSILE_HEIGHT);
    missileDirection = missileDirection - position;
    missileDirection.normalise();

    double level = static_cast<double>(creature->getLevel());
    double phyAtk = mPhyAtk + (level * mPhyAtkPerLvl);
    double magAtk = mMagAtk + (level * mMagAtkPerLvl);
    double eleAtk = mMagAtk + (level * mEleAtkPerLvl);
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

    MissileOneHit* missile = new MissileOneHit(&gameMap, gameMap.isServerGameMap(), creature->getSeat(),
        creature->getName(), mMissileMesh, mMissilePartScript, missileDirection, mMissileSpeed, phyAtk, magAtk, eleAtk,
        attackedObject, false);
    missile->addToGameMap();
    missile->createMesh();
    missile->setPosition(position);
    // We don't want the missile to stay idle for 1 turn. Because we are in a doUpkeep context,
    // we can safely call the missile doUpkeep as we know the engine will not call it the turn
    // it has been added
    missile->doUpkeep();

    return true;
}

CreatureSkillMissileLaunch* CreatureSkillMissileLaunch::clone() const
{
    return new CreatureSkillMissileLaunch(*this);
}

void CreatureSkillMissileLaunch::getFormatString(std::string& format) const
{
    CreatureSkill::getFormatString(format);
    if(!format.empty())
        format += "\t";

    format += "RangeMax\tRangePerLvl\tLevelMin\tMissileMesh\tMissilePartScript\tMissileSpeed\tPhyAtk\tPhyAtkPerLvl\tMagAtk\tMagAtkPerLvl\tEleAtk\tEleAtkPerLvl";

}

void CreatureSkillMissileLaunch::exportToStream(std::ostream& os) const
{
    CreatureSkill::exportToStream(os);
    os << "\t" << mRangeMax;
    os << "\t" << mRangePerLvl;
    os << "\t" << mCreatureLevelMin;
    if(mMissileMesh.empty())
        os << "\tnone";
    else
        os << "\t" << mMissileMesh;
    if(mMissilePartScript.empty())
        os << "\tnone";
    else
        os << "\t" << mMissilePartScript;
    os << "\t" << mMissileSpeed;
    os << "\t" << mPhyAtk;
    os << "\t" << mPhyAtkPerLvl;
    os << "\t" << mMagAtk;
    os << "\t" << mMagAtkPerLvl;
    os << "\t" << mEleAtk;
    os << "\t" << mEleAtkPerLvl;
}

bool CreatureSkillMissileLaunch::importFromStream(std::istream& is)
{
    if(!CreatureSkill::importFromStream(is))
        return false;

    if(!(is >> mRangeMax))
        return false;
    if(!(is >> mRangePerLvl))
        return false;
    if(!(is >> mCreatureLevelMin))
        return false;
    if(!(is >> mMissileMesh))
        return false;
    if(mMissileMesh == "none")
        mMissileMesh.clear();
    if(!(is >> mMissilePartScript))
        return false;
    if(mMissilePartScript == "none")
        mMissilePartScript.clear();
    if(!(is >> mMissileSpeed))
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

bool CreatureSkillMissileLaunch::isEqual(const CreatureSkill& creatureSkill) const
{
    if(!CreatureSkill::isEqual(creatureSkill))
        return false;

    const CreatureSkillMissileLaunch* skill = dynamic_cast<const CreatureSkillMissileLaunch*>(&creatureSkill);
    if(skill == nullptr)
        return false;

    if(mRangeMax != skill->mRangeMax)
        return false;
    if(mRangePerLvl != skill->mRangePerLvl)
        return false;
    if(mCreatureLevelMin != skill->mCreatureLevelMin)
        return false;
    if(mMissileMesh != skill->mMissileMesh)
        return false;
    if(mMissilePartScript != skill->mMissilePartScript)
        return false;
    if(mMissileSpeed != skill->mMissileSpeed)
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
