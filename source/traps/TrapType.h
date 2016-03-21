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

#ifndef TRAPTYPE_H
#define TRAPTYPE_H

#include <iosfwd>
#include <string>

class ODPacket;

enum class TrapType
{
    nullTrapType = 0,
    cannon,
    spike,
    boulder,
    doorWooden,
    nbTraps     // Must be the last in this enum
};

std::istream& operator>>(std::istream& is, TrapType& tt);
std::ostream& operator<<(std::ostream& os, const TrapType& tt);
ODPacket& operator>>(ODPacket& is, TrapType& tt);
ODPacket& operator<<(ODPacket& os, const TrapType& tt);

#endif // TRAPTYPE_H
