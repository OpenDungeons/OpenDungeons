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

#ifndef CREATUREMOOD_H
#define CREATUREMOOD_H

#include <cstdint>
#include <iosfwd>
#include <string>

class Creature;
class GameMap;

enum class CreatureMoodLevel
{
    Happy,
    Neutral,
    Upset,
    Angry,
    Furious
};

class CreatureMood
{
public:
    // Constructors
    CreatureMood()
    {}

    virtual ~CreatureMood()
    {}

    virtual const std::string& getModifierName() const = 0;

    //! \brief Computes the creature mood for this modifier
    virtual int32_t computeMood(const Creature& creature) const = 0;

    //! \brief This function should return a copy of the current class
    virtual CreatureMood* clone() const = 0;

    virtual bool isEqual(const CreatureMood& creatureMood) const;

    //! \brief Returns the format string (including specific additional parameters)
    virtual void getFormatString(std::string& format) const
    {}

    //! \brief Can be overriden to read additional parameters from the stream
    virtual bool importFromStream(std::istream& file)
    { return true; }
    virtual void exportToStream(std::ostream& os) const
    {}

    static std::string toString(CreatureMoodLevel moodLevel);
};

#endif // CREATUREMOOD_H
