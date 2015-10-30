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

#ifndef CREATURESKILLHASTESELF_H
#define CREATURESKILLHASTESELF_H

#include "creatureskill/CreatureSkill.h"

#include <cstdint>
#include <iosfwd>
#include <string>

class Creature;
class GameMap;

class CreatureSkillHasteSelf : public CreatureSkill
{
public:
    // Constructors
    CreatureSkillHasteSelf() :
        mCreatureLevelMin(0),
        mEffectDuration(0),
        mEffectValue(0.0)
    {}

    virtual ~CreatureSkillHasteSelf()
    {}

    virtual const std::string& getSkillName() const override;

    virtual bool canBeUsedBy(const Creature* creature) const override;

    virtual bool tryUseSupport(GameMap& gameMap, Creature* creature) const override;

    virtual bool tryUseFight(GameMap& gameMap, Creature* creature, float range,
        GameEntity* attackedObject, Tile* attackedTile) const override
    { return false; }

    virtual CreatureSkillHasteSelf* clone() const override;

    virtual bool isEqual(const CreatureSkill& creatureSkill) const override;

    virtual void getFormatString(std::string& format) const;
    virtual void exportToStream(std::ostream& os) const override;
    virtual bool importFromStream(std::istream& is) override;

private:
    uint32_t mCreatureLevelMin;
    uint32_t mEffectDuration;
    double mEffectValue;

};

#endif // CREATURESKILLHASTESELF_H
