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

#ifndef _LOGSINKFILE_H_
#define _LOGSINKFILE_H_

#include <fstream>

#include "LogSink.h"

class LogSinkFile : public LogSink
{
public:
    LogSinkFile(const std::string& filepath);
    ~LogSinkFile();

    virtual void write(LogMessageLevel level, const std::string& module, const std::string& timestamp, const std::string& filename, int line, const char* message) override;
private:
    std::ofstream mFile;
};

#endif // _LOGSINKFILE_H_
