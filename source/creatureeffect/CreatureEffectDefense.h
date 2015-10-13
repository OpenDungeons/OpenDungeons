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

#ifndef CREATUREEFFECTDEFENSE_H
#define CREATUREEFFECTDEFENSE_H

#include "creatureeffect/CreatureEffect.h"

class CreatureEffectDefense : public CreatureEffect
{
public:
    CreatureEffectDefense(uint32_t nbTurnsEffect, double phy, double mag, double ele, const std::string& particleEffectName) :
        CreatureEffect(nbTurnsEffect, particleEffectName),
        mPhy(phy),
        mMag(mag),
        mEle(ele)
    {}

    CreatureEffectDefense() :
        CreatureEffect(),
        mPhy(0.0),
        mMag(0.0),
        mEle(0.0)
    {}

    virtual ~CreatureEffectDefense()
    {}

    virtual const std::string& getEffectName() const override;

    virtual void exportToStream(std::ostream& os) const override;
    virtual bool importFromStream(std::istream& is) override;

    static CreatureEffectDefense* load(std::istream& is);

protected:
    virtual void applyEffect(Creature& creature) override;
    virtual void releaseEffect(Creature& creature) override;

private:
    double mPhy;
    double mMag;
    double mEle;
};

#endif // CREATUREEFFECTDEFENSE_H
