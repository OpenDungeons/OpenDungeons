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

#include "creatureskill/CreatureSkillDefenseSelf.h"

#include "creatureaction/CreatureAction.h"
#include "creatureeffect/CreatureEffectDefense.h"
#include "creatureskill/CreatureSkillManager.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "gamemap/GameMap.h"
#include "sound/SoundEffectsManager.h"

#include <istream>

const std::string CreatureSkillDefenseSelfName = "DefenseSelf";

namespace
{
class CreatureSkillDefenseSelfFactory : public CreatureSkillFactory
{
    CreatureSkill* createCreatureSkill() const override
    { return new CreatureSkillDefenseSelf; }

    const std::string& getCreatureSkillName() const override
    { return CreatureSkillDefenseSelfName; }
};

// Register the factory
static CreatureSkillRegister reg(new CreatureSkillDefenseSelfFactory);
}

const std::string& CreatureSkillDefenseSelf::getSkillName() const
{
    return CreatureSkillDefenseSelfName;
}

bool CreatureSkillDefenseSelf::canBeUsedBy(const Creature* creature) const
{
    if(creature->getLevel() < mCreatureLevelMin)
        return false;

    return true;
}

bool CreatureSkillDefenseSelf::tryUseSupport(GameMap& gameMap, Creature* creature) const
{
    if(!creature->isAlive())
        return false;

    // We only cast the spell while fighting
    if(!creature->isActionInList(CreatureActionType::fight))
        return false;

    CreatureEffectDefense* effect = new CreatureEffectDefense(mEffectDuration, mPhy, mMag, mEle, "SpellCreatureDefense");
    creature->addCreatureEffect(effect);

    for(Tile* tile : creature->getCoveredTiles())
    {
        gameMap.fireSpatialSound(tile->getSeatsWithVision(), SpatialSoundType::Spells, "Defense", tile);
    }

    return true;
}

CreatureSkillDefenseSelf* CreatureSkillDefenseSelf::clone() const
{
    return new CreatureSkillDefenseSelf(*this);
}

void CreatureSkillDefenseSelf::getFormatString(std::string& format) const
{
    CreatureSkill::getFormatString(format);
    if(!format.empty())
        format += "\t";

    format += "LevelMin\tEffectDuration\tPhyDef\tMagDef\tEleDef";
}

void CreatureSkillDefenseSelf::exportToStream(std::ostream& os) const
{
    CreatureSkill::exportToStream(os);
    os << "\t" << mCreatureLevelMin;
    os << "\t" << mEffectDuration;
    os << "\t" << mPhy;
    os << "\t" << mMag;
    os << "\t" << mEle;
}

bool CreatureSkillDefenseSelf::importFromStream(std::istream& is)
{
    if(!CreatureSkill::importFromStream(is))
        return false;

    if(!(is >> mCreatureLevelMin))
        return false;
    if(!(is >> mEffectDuration))
        return false;
    if(!(is >> mPhy))
        return false;
    if(!(is >> mMag))
        return false;
    if(!(is >> mEle))
        return false;

    return true;
}

bool CreatureSkillDefenseSelf::isEqual(const CreatureSkill& creatureSkill) const
{
    if(!CreatureSkill::isEqual(creatureSkill))
        return false;

    const CreatureSkillDefenseSelf* skill = dynamic_cast<const CreatureSkillDefenseSelf*>(&creatureSkill);
    if(skill == nullptr)
        return false;

    if(mCreatureLevelMin != skill->mCreatureLevelMin)
        return false;
    if(mEffectDuration != skill->mEffectDuration)
        return false;
    if(mPhy != skill->mPhy)
        return false;
    if(mMag != skill->mMag)
        return false;
    if(mEle != skill->mEle)
        return false;

    return true;
}
