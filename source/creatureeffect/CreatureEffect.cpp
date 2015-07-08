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

#include "creatureeffect/CreatureEffect.h"

#include "creatureeffect/CreatureEffectHeal.h"
#include "creatureeffect/CreatureEffectExplosion.h"
#include "creatureeffect/CreatureEffectSpeedChange.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <istream>

void CreatureEffect::write(const CreatureEffect& effect, std::ostream& os)
{
    os << effect.getCreatureEffectType() << "\t";
    effect.exportToStream(os);
}

CreatureEffect* CreatureEffect::load(std::istream& is)
{
    CreatureEffectType effectType;
    OD_ASSERT_TRUE(is >> effectType);
    switch(effectType)
    {
        case CreatureEffectType::heal:
            return CreatureEffectHeal::load(is);
        case CreatureEffectType::speedChange:
            return CreatureEffectSpeedChange::load(is);
        case CreatureEffectType::explosion:
            return CreatureEffectExplosion::load(is);
        default:
            OD_LOG_ERR("Unallowed enum value=" + Helper::toString(static_cast<int32_t>(effectType)));
            return nullptr;
    }
}

bool CreatureEffect::upkeepEffect(Creature& creature)
{
    if(mNbTurnsEffect <= 0)
    {
        releaseEffect(creature);
        return false;
    }

    --mNbTurnsEffect;
    applyEffect(creature);
    return true;
}

void CreatureEffect::exportToStream(std::ostream& os) const
{
    os << mNbTurnsEffect << "\t";
    os << mParticleEffectScript;
}

void CreatureEffect::importFromStream(std::istream& is)
{
    OD_ASSERT_TRUE(is >> mNbTurnsEffect);
    OD_ASSERT_TRUE(is >> mParticleEffectScript);
}

std::ostream& operator<<(std::ostream& os, const CreatureEffectType& type)
{
    os << static_cast<int32_t>(type);
    return os;
}

std::istream& operator>>(std::istream& is, CreatureEffectType& type)
{
    int32_t tmp;
    OD_ASSERT_TRUE(is >> tmp);
    type = static_cast<CreatureEffectType>(tmp);
    return is;
}
