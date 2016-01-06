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

#include "creatureeffect/CreatureEffectSlap.h"

#include "creatureeffect/CreatureEffectManager.h"
#include "entities/Creature.h"
#include "utils/LogManager.h"

static const std::string CreatureEffectSlapName = "Slap";

namespace
{
class CreatureEffectSlapFactory : public CreatureEffectFactory
{
    CreatureEffect* createCreatureEffect() const override
    { return new CreatureEffectSlap; }

    const std::string& getCreatureEffectName() const override
    {
        return CreatureEffectSlapName;
    }
};

// Register the factory
static CreatureEffectRegister reg(new CreatureEffectSlapFactory);
}

const std::string& CreatureEffectSlap::getEffectName() const
{
    return CreatureEffectSlapName;
}

void CreatureEffectSlap::startEffect(Creature& creature)
{
    creature.addActiveSlapCount();
}

void CreatureEffectSlap::releaseEffect(Creature& creature)
{
    creature.removeActiveSlapCount();
}

void CreatureEffectSlap::applyEffect(Creature& creature)
{
    // We do not do anything. Having a CreatureEffectSlap in its effect queue is enough
    // to change the creature behaviour
}

CreatureEffectSlap* CreatureEffectSlap::load(std::istream& is)
{
    CreatureEffectSlap* effect = new CreatureEffectSlap;
    effect->importFromStream(is);
    return effect;
}
