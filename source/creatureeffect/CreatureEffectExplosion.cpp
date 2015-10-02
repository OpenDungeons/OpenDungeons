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

#include "creatureeffect/CreatureEffectExplosion.h"

#include "entities/Creature.h"
#include "utils/LogManager.h"

void CreatureEffectExplosion::applyEffect(Creature& creature)
{
    if(!creature.isAlive())
        return;

    Tile* posTile = creature.getPositionTile();
    creature.takeDamage(nullptr, 0.0, mEffectValue, 0.0, posTile, false, false, false);
}

CreatureEffectExplosion* CreatureEffectExplosion::load(std::istream& is)
{
    CreatureEffectExplosion* effect = new CreatureEffectExplosion;
    effect->importFromStream(is);
    return effect;
}

void CreatureEffectExplosion::exportToStream(std::ostream& os) const
{
    CreatureEffect::exportToStream(os);
    os << "\t" << mEffectValue;
}

void CreatureEffectExplosion::importFromStream(std::istream& is)
{
    CreatureEffect::importFromStream(is);
    OD_ASSERT_TRUE(is >> mEffectValue);
}
