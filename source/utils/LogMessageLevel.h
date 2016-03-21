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

#ifndef _LOGMESSAGELEVEL_H_
#define _LOGMESSAGELEVEL_H_

enum class LogMessageLevel
{
    TRIVIAL,
    NORMAL,
    WARNING,
    CRITICAL,
    NB_LEVELS
};

inline const char* LogMessageLevelToString(LogMessageLevel level)
{
    switch(level)
    {
        case LogMessageLevel::TRIVIAL:
            return "TRIVIAL";
        case LogMessageLevel::NORMAL:
            return "NORMAL";
        case LogMessageLevel::WARNING:
            return "WARNING";
        case LogMessageLevel::CRITICAL:
            return "CRITICAL";
        default:
            return "<Invalid>";
    }
}

#endif // _LOGMESSAGELEVEL_H_
