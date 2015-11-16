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

#ifndef CREATURESKILLMELEEFIGHT_H
#define CREATURESKILLMELEEFIGHT_H

#include "creatureskill/CreatureSkill.h"

#include <cstdint>
#include <iosfwd>
#include <string>

class Creature;
class GameMap;

class CreatureSkillMeleeFight : public CreatureSkill
{
public:
    // Constructors
    CreatureSkillMeleeFight() :
        mCreatureLevelMin(0),
        mPhyAtk(0.0),
        mPhyAtkPerLvl(0.0),
        mMagAtk(0.0),
        mMagAtkPerLvl(0.0),
        mEleAtk(0.0),
        mEleAtkPerLvl(0.0)
    {}

    virtual ~CreatureSkillMeleeFight()
    {}

    virtual const std::string& getSkillName() const override;

    virtual double getRangeMax(const Creature* creature, GameEntity* entityAttack) const override
    { return 1.0; }

    virtual bool canBeUsedBy(const Creature* creature) const override;

    virtual bool tryUseSupport(GameMap& gameMap, Creature* creature) const override
    { return false; }

    virtual bool tryUseFight(GameMap& gameMap, Creature* creature, float range,
        GameEntity* attackedObject, Tile* attackedTile, bool ko) const override;

    virtual CreatureSkillMeleeFight* clone() const override;

    virtual bool isEqual(const CreatureSkill& creatureSkill) const override;

    virtual void getFormatString(std::string& format) const;
    virtual void exportToStream(std::ostream& os) const override;
    virtual bool importFromStream(std::istream& is) override;

private:
    uint32_t mCreatureLevelMin;
    double mPhyAtk;
    double mPhyAtkPerLvl;
    double mMagAtk;
    double mMagAtkPerLvl;
    double mEleAtk;
    double mEleAtkPerLvl;

};

#endif // CREATURESKILLMELEEFIGHT_H
