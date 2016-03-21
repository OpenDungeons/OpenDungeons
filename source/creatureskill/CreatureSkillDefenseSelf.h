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

#ifndef CREATURESKILLDEFENSESELF_H
#define CREATURESKILLDEFENSESELF_H

#include "creatureskill/CreatureSkill.h"

#include <cstdint>
#include <iosfwd>
#include <string>

class Creature;
class GameMap;

class CreatureSkillDefenseSelf : public CreatureSkill
{
public:
    // Constructors
    CreatureSkillDefenseSelf() :
        mCreatureLevelMin(0),
        mEffectDuration(0),
        mPhy(0.0),
        mMag(0.0),
        mEle(0.0)
    {}

    virtual ~CreatureSkillDefenseSelf()
    {}

    virtual const std::string& getSkillName() const override;

    virtual bool canBeUsedBy(const Creature* creature) const override;

    virtual bool tryUseSupport(GameMap& gameMap, Creature* creature) const override;

    virtual bool tryUseFight(GameMap& gameMap, Creature* creature, float range,
        GameEntity* attackedObject, Tile* attackedTile, bool ko) const override
    { return false; }

    virtual CreatureSkillDefenseSelf* clone() const override;

    virtual bool isEqual(const CreatureSkill& creatureSkill) const override;

    virtual void getFormatString(std::string& format) const;
    virtual void exportToStream(std::ostream& os) const override;
    virtual bool importFromStream(std::istream& is) override;

private:
    uint32_t mCreatureLevelMin;
    uint32_t mEffectDuration;
    double mPhy;
    double mMag;
    double mEle;

};

#endif // CREATURESKILLDEFENSESELF_H
