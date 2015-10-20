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

#ifndef CREATUREBEHAVIOUR_H
#define CREATUREBEHAVIOUR_H

#include <cstdint>
#include <iosfwd>
#include <string>

class Creature;

class CreatureBehaviour
{
public:
    CreatureBehaviour()
    {}

    virtual ~CreatureBehaviour()
    {}

    virtual const std::string& getName() const = 0;

    //! \brief Does what the behaviour needs to do. Note that creature behaviour
    //! are processed during upkeep start. It is a plugin to allow creatures to
    //! behave differently.
    //! This can be done by pushing actions depending on what happens with the creature
    //! If it returns true, the behaviours will continue to be processed. If false,
    //! the behaviours will stop being processed.
    //! That implies that the behaviour order matters
    virtual bool processBehaviour(Creature& creature) const = 0;

    //! \brief returns a new instance of the given creature behaviour. That is needed to
    //! duplicate CreatureDefinition class
    virtual CreatureBehaviour* clone() const = 0;

    virtual bool isEqual(const CreatureBehaviour& creatureBehaviour) const;

    //! \brief Returns the format string (including specific additional parameters)
    virtual void getFormatString(std::string& format) const
    {}

    //! \brief Write skill data to the given stream
    virtual void exportToStream(std::ostream& os) const
    {}

    //! \brief Read skills data. Returns true if loading is OK and false otherwise
    virtual bool importFromStream(std::istream& is)
    { return true; }
};

#endif // CREATUREBEHAVIOUR_H
