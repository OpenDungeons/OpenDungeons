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

#include "creatureeffect/CreatureEffectDefense.h"

#include "creatureeffect/CreatureEffectManager.h"
#include "entities/Creature.h"
#include "utils/LogManager.h"

static const std::string CreatureEffectDefenseName = "Defense";

namespace
{
class CreatureEffectDefenseFactory : public CreatureEffectFactory
{
    CreatureEffect* createCreatureEffect() const override
    { return new CreatureEffectDefense; }

    const std::string& getCreatureEffectName() const override
    {
        return CreatureEffectDefenseName;
    }
};

// Register the factory
static CreatureEffectRegister reg(new CreatureEffectDefenseFactory);
}

const std::string& CreatureEffectDefense::getEffectName() const
{
    return CreatureEffectDefenseName;
}

void CreatureEffectDefense::applyEffect(Creature& creature)
{
    if(!creature.isAlive())
        return;

    creature.setDefenseModifier(mPhy, mMag, mEle);
}

void CreatureEffectDefense::releaseEffect(Creature& creature)
{
    if(!creature.isAlive())
        return;

    creature.clearDefenseModifier();
    mPhy = 0.0;
    mMag = 0.0;
    mEle = 0.0;
}

CreatureEffectDefense* CreatureEffectDefense::load(std::istream& is)
{
    CreatureEffectDefense* effect = new CreatureEffectDefense;
    effect->importFromStream(is);
    return effect;
}

void CreatureEffectDefense::exportToStream(std::ostream& os) const
{
    CreatureEffect::exportToStream(os);
    os << "\t" << mPhy;
    os << "\t" << mMag;
    os << "\t" << mEle;
}

bool CreatureEffectDefense::importFromStream(std::istream& is)
{
    if(!CreatureEffect::importFromStream(is))
        return false;
    if(!(is >> mPhy))
        return false;
    if(!(is >> mMag))
        return false;
    if(!(is >> mEle))
        return false;

    return true;
}
