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

#include "utils/LogSinkConsole.h"

#include <iostream>
#include <sstream>

#if WIN32 || _WINDOWS
    #include <Windows.h>
#endif

LogSinkConsole::LogSinkConsole()
{

}

LogSinkConsole::~LogSinkConsole()
{

}

void LogSinkConsole::write(LogMessageLevel level, const std::string& module, const std::string& timestamp, const std::string& filename, int line, const std::string& message)
{
    std::stringstream ss;

    ss
        << "(" << timestamp << ") "
        << "(" << module << ") "
        << "[" << LogMessageLevelToString(level) << "] ";

    if (level >= LogMessageLevel::WARNING)
        ss << "(" << filename << ":" << line << ") ";

    ss
        << message
        << std::endl;

    if (level >= LogMessageLevel::WARNING)
        std::cerr << ss.str();
    else
        std::cout << ss.str();

#if (WIN32 || _WINDOWS) && OD_DEBUG
    ::OutputDebugStringA(ss.str().c_str());
#endif
}
