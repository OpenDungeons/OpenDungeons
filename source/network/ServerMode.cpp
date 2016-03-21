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

#include "network/ServerMode.h"

#include "network/ODPacket.h"
#include "utils/Helper.h"

namespace ServerModes
{
    std::string toString(ServerMode sm)
    {
        switch(sm)
        {
            case ServerMode::ModeNone:
                return "ModeNone";
            case ServerMode::ModeGameSinglePlayer:
                return "ModeGameSinglePlayer";
            case ServerMode::ModeGameMultiPlayer:
                return "ModeGameMultiPlayer";
            case ServerMode::ModeGameLoaded:
                return "ModeGameLoaded";
            case ServerMode::ModeEditor:
                return "ModeEditor";
            default:
                return "Unexpected ServerMode=" + Helper::toString(static_cast<uint32_t>(sm));
        }
    }
}

ODPacket& operator<<(ODPacket& os, const ServerMode& sm)
{
    os << static_cast<int32_t>(sm);
    return os;
}

ODPacket& operator>>(ODPacket& is, ServerMode& sm)
{
    int32_t tmp;
    is >> tmp;
    sm = static_cast<ServerMode>(tmp);
    return is;
}
