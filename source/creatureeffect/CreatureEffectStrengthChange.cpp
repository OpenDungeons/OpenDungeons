/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#include "creatureeffect/CreatureEffectStrengthChange.h"

#include "creatureeffect/CreatureEffectManager.h"
#include "entities/Creature.h"
#include "utils/LogManager.h"

static const std::string CreatureEffectStrengthChangeName = "StrengthChange";

namespace
{
class CreatureEffectStrengthChangeFactory : public CreatureEffectFactory
{
    CreatureEffect* createCreatureEffect() const override
    { return new CreatureEffectStrengthChange; }

    const std::string& getCreatureEffectName() const override
    {
        return CreatureEffectStrengthChangeName;
    }
};

// Register the factory
static CreatureEffectRegister reg(new CreatureEffectStrengthChangeFactory);
}

const std::string& CreatureEffectStrengthChange::getEffectName() const
{
    return CreatureEffectStrengthChangeName;
}

void CreatureEffectStrengthChange::applyEffect(Creature& creature)
{
    if(!creature.isAlive())
        return;

    if(mEffectValue == 1.0)
        return;

    creature.setStrengthModifier(mEffectValue);
    mEffectValue = 1.0;
}

void CreatureEffectStrengthChange::releaseEffect(Creature& creature)
{
    if(!creature.isAlive())
        return;

    creature.clearStrengthModifier();
    mEffectValue = 1.0;
}

CreatureEffectStrengthChange* CreatureEffectStrengthChange::load(std::istream& is)
{
    CreatureEffectStrengthChange* effect = new CreatureEffectStrengthChange;
    effect->importFromStream(is);
    return effect;
}

void CreatureEffectStrengthChange::exportToStream(std::ostream& os) const
{
    CreatureEffect::exportToStream(os);
    os << "\t" << mEffectValue;
}

bool CreatureEffectStrengthChange::importFromStream(std::istream& is)
{
    if(!CreatureEffect::importFromStream(is))
        return false;
    if(!(is >> mEffectValue))
        return false;

    return true;
}
