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

#ifndef CREATUREEFFECTSLAP_H
#define CREATUREEFFECTSLAP_H

#include "creatureeffect/CreatureEffect.h"

class CreatureEffectSlap : public CreatureEffect
{
public:
    CreatureEffectSlap(uint32_t nbTurnsEffect, const std::string& particleEffectScript) :
        CreatureEffect(nbTurnsEffect, particleEffectScript)
    {}

    CreatureEffectSlap() :
        CreatureEffect()
    {}

    virtual ~CreatureEffectSlap()
    {}

    virtual CreatureEffectType getCreatureEffectType() const override
    { return CreatureEffectType::slap; }

    virtual const std::string& getEffectName() const override;

    static CreatureEffectSlap* load(std::istream& is);

protected:
    virtual void applyEffect(Creature& creature) override;
};

#endif // CREATUREEFFECTSLAP_H
