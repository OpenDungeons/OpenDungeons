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

#include "Command.h"

#include <algorithm>

Command::Result Command::executeClient(const ArgumentList_t& argumentList, ModeType mode,
                                 ConsoleInterface& consoleInterface, AbstractModeManager& modeManager)
{
    if(isAllowedInMode(mode))
    {
        return mCommandClient(argumentList, consoleInterface, modeManager);
    }
    return Result::WRONG_MODE;
}

Command::Result Command::executeServer(const ArgumentList_t& args, ConsoleInterface& c, GameMap& gameMap)
{
    return mCommandServer(args, c, gameMap);
}
