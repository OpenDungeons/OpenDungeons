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

#include "TrapType.h"

#include "network/ODPacket.h"
#include "utils/Helper.h"

#include <istream>
#include <ostream>

std::istream& operator>>(std::istream& is, TrapType& tt)
{
    uint32_t tmp;
    is >> tmp;
    tt = static_cast<TrapType>(tmp);
    return is;
}

std::ostream& operator<<(std::ostream& os, const TrapType& tt)
{
    uint32_t tmp = static_cast<uint32_t>(tt);
    os << tmp;
    return os;
}

ODPacket& operator>>(ODPacket& is, TrapType& tt)
{
    uint32_t tmp;
    is >> tmp;
    tt = static_cast<TrapType>(tmp);
    return is;
}

ODPacket& operator<<(ODPacket& os, const TrapType& tt)
{
    uint32_t tmp = static_cast<uint32_t>(tt);
    os << tmp;
    return os;
}

namespace Traps
{
std::string getTrapNameFromTrapType(TrapType t)
{
    switch (t)
    {
        case TrapType::nullTrapType:
            return "NullTrapType";

        case TrapType::cannon:
            return "Cannon";

        case TrapType::spike:
            return "Spike";

        case TrapType::boulder:
            return "Boulder";

        default:
            return "UnknownTrapType enum=" + Helper::toString(static_cast<int>(t));
    }
}
}
