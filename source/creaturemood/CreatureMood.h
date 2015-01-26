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

#ifndef CREATUREMOOD_H
#define CREATUREMOOD_H

#include <cstdint>
#include <istream>
#include <vector>

class Creature;
class GameMap;

class CreatureMood
{
public:
    enum CreatureMoodType
    {
        unknown,
        awakness,
        creature,
        fee,
        hploss,
        hunger
    };

    // Constructors
    CreatureMood()
    {}

    virtual ~CreatureMood()
    {}

    virtual CreatureMoodType getCreatureMoodType() const
    { return CreatureMoodType::unknown; }

    //! \brief Computes the creature mood for this modifier
    virtual int32_t computeMood(const Creature* creature) const = 0;

    //! \brief This function should return a copy of the current class
    virtual CreatureMood* clone() const = 0;

    //! \brief This function will be called after loading the level
    virtual void init(GameMap* gameMap)
    {}

    static CreatureMood* load(std::istream& defFile);
};

#endif // CREATUREMOOD_H
