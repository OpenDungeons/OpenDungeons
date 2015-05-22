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

#ifndef LOGMANAGERFILE_H
#define LOGMANAGERFILE_H

#include "utils/LogManager.h"

//! \brief Logger implementation using the logging system from boost
class LogManagerFile final : public LogManager
{
public:
    LogManagerFile(const std::string& userDataPath);
    void logMessage(const std::string& message, LogMessageLevel lml = LogMessageLevel::NORMAL,
                    bool maskDebug = false, bool addTimeStamp = false) override;
    void setLogDetail(LogMessageLevel ll) override;
private:
    std::ofstream	mFileLog;
    LogMessageLevel mLevel;
};

#endif // LOGMANAGERFILE_H
