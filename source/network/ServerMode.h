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

#ifndef SERVERMODE_H
#define SERVERMODE_H

#include <string>

class ODPacket;

enum class ServerMode
{
    ModeNone,
    ModeGameSinglePlayer,
    ModeGameMultiPlayer,
    ModeGameLoaded,
    ModeEditor
};

ODPacket& operator<<(ODPacket& os, const ServerMode& sm);
ODPacket& operator>>(ODPacket& is, ServerMode& sm);

namespace ServerModes
{
    std::string toString(ServerMode sm);
}

#endif // SERVERMODE_H

