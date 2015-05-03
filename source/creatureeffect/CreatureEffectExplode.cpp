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

#include "creatureeffect/CreatureEffectExplode.h"

#include "entities/Creature.h"
#include "utils/LogManager.h"

void CreatureEffectExplode::applyEffect(Creature& creature)
{
    if(creature.getHP() <= 0.0)
        return;

    Tile* posTile = creature.getPositionTile();
    creature.takeDamage(nullptr, 0.0, mEffectValue, posTile);
}

CreatureEffectExplode* CreatureEffectExplode::load(std::istream& is)
{
    CreatureEffectExplode* effect = new CreatureEffectExplode;
    effect->importFromStream(is);
    return effect;
}

void CreatureEffectExplode::exportToStream(std::ostream& os) const
{
    CreatureEffect::exportToStream(os);
    os << "\t" << mEffectValue;
}

void CreatureEffectExplode::importFromStream(std::istream& is)
{
    CreatureEffect::importFromStream(is);
    OD_ASSERT_TRUE(is >> mEffectValue);
}
