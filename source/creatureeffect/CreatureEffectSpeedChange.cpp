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

#include "entities/Creature.h"
#include "utils/LogManager.h"

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

    creature.setMoveSpeedModifier(1.0);
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

void CreatureEffectSpeedChange::importFromStream(std::istream& is)
{
    CreatureEffect::importFromStream(is);
    OD_ASSERT_TRUE(is >> mEffectValue);
}
