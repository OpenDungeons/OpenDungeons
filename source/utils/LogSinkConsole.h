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

#ifndef _LOGSINKCONSOLE_H_
#define _LOGSINKCONSOLE_H_

#include "utils/LogSink.h"

class LogSinkConsole : public LogSink
{
public:
    LogSinkConsole();
    ~LogSinkConsole();

    virtual void write(LogMessageLevel level, const std::string& module, const std::string& timestamp, const std::string& filename, int line, const char* message) override;
};

#endif // _LOGSINKCONSOLE_H_
