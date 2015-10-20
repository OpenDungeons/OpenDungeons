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

#include "creatureeffect/CreatureEffectSpeedChange.h"

#include "creatureeffect/CreatureEffectManager.h"
#include "entities/Creature.h"
#include "utils/LogManager.h"

static const std::string CreatureEffectSpeedChangeName = "SpeedChange";

namespace
{
class CreatureEffectSpeedChangeFactory : public CreatureEffectFactory
{
    CreatureEffect* createCreatureEffect() const override
    { return new CreatureEffectSpeedChange; }

    const std::string& getCreatureEffectName() const override
    {
        return CreatureEffectSpeedChangeName;
    }
};

// Register the factory
static CreatureEffectRegister reg(new CreatureEffectSpeedChangeFactory);
}

const std::string& CreatureEffectSpeedChange::getEffectName() const
{
    return CreatureEffectSpeedChangeName;
}

void CreatureEffectSpeedChange::applyEffect(Creature& creature)
{
    if(!creature.isAlive())
        return;

    if(mEffectValue == 1.0)
        return;

    creature.setMoveSpeedModifier(mEffectValue);
    mEffectValue = 1.0;
}

void CreatureEffectSpeedChange::releaseEffect(Creature& creature)
{
    if(!creature.isAlive())
        return;

    creature.clearMoveSpeedModifier();
    mEffectValue = 1.0;

}

CreatureEffectSpeedChange* CreatureEffectSpeedChange::load(std::istream& is)
{
    CreatureEffectSpeedChange* effect = new CreatureEffectSpeedChange;
    effect->importFromStream(is);
    return effect;
}

void CreatureEffectSpeedChange::exportToStream(std::ostream& os) const
{
    CreatureEffect::exportToStream(os);
    os << "\t" << mEffectValue;
}

bool CreatureEffectSpeedChange::importFromStream(std::istream& is)
{
    if(!CreatureEffect::importFromStream(is))
        return false;
    if(!(is >> mEffectValue))
        return false;

    return true;
}
