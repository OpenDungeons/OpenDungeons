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

#ifndef CREATURESKILL_H
#define CREATURESKILL_H

#include <cstdint>
#include <iosfwd>
#include <string>

class Creature;
class GameEntity;
class GameMap;
class Tile;

//! \brief Defines the skills the creatures can use in game. Note that the CreatureSkill
//! belongs to the creature definition. Thus, all its functions are shared and should be const
class CreatureSkill
{
public:
    // Constructors
    CreatureSkill() :
        mCooldownNbTurns(0),
        mWarmupNbTurns(0)
    {}

    virtual ~CreatureSkill()
    {}

    virtual const std::string& getSkillName() const = 0;

    inline uint32_t getCooldownNbTurns() const
    { return mCooldownNbTurns; }

    inline uint32_t getWarmupNbTurns() const
    { return mWarmupNbTurns; }

    //! \brief returns true is the required needs (like level) are met for the given
    //! creature. false otherwise
    virtual bool canBeUsedBy(const Creature* creature) const = 0;

    //! \brief To check if it can attack, the creature needs to know the range it can reach.
    //! This function returns the max range which the creature can use. It can return 0 if
    //! no range
    virtual double getRangeMax(const Creature* creature, GameEntity* entityAttack) const
    { return 0; }

    //! \brief Tries to use the skill during support (at beginning of each turn).
    //! Note that every skill is tested for support at the beginning of each turn
    //! Returns true if the skill was used and false otherwise
    virtual bool tryUseSupport(GameMap& gameMap, Creature* creature) const = 0;

    //! \brief Tries to launch the skill. Returns true if the skill could be used.
    //! Note that only one skill can be launched at each turn. That means that the
    //! skill order matters as the first one is more likely to be used than the others
    //! Returns true if the skill was used and false otherwise
    virtual bool tryUseFight(GameMap& gameMap, Creature* creature, float range,
        GameEntity* attackedObject, Tile* attackedTile, bool ko) const = 0;

    //! \brief returns a new instance of the given creature skill. That is needed to
    //! duplicate CreatureDefinition class
    virtual CreatureSkill* clone() const = 0;

    virtual bool isEqual(const CreatureSkill& creatureSkill) const;

    //! \brief Returns the format string (including specific additional parameters)
    virtual void getFormatString(std::string& format) const;

    //! \brief Write skill data to the given stream
    virtual void exportToStream(std::ostream& os) const;
    //! \brief Read skills data. Returns true if loading is OK and false otherwise
    virtual bool importFromStream(std::istream& is);

private:
    uint32_t mCooldownNbTurns;
    uint32_t mWarmupNbTurns;
};

#endif // CREATURESKILL_H
