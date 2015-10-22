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

#ifndef _LOGSINKOGRE_H_
#define _LOGSINKOGRE_H_

#include <mutex>

#include <OgreConfig.h>

#include "utils/LogSink.h"

namespace Ogre {

    class Log;
    class LogManager;

};

class LogSinkOgre : public LogSink
{
public:
    LogSinkOgre(const std::string& userDataPath);
    ~LogSinkOgre();

    virtual void write(LogMessageLevel level, const std::string& module, const std::string& timestamp, const std::string& filename, int line, const std::string& message) override;
private:
    std::unique_ptr<Ogre::LogManager> mLogManager;
    Ogre::Log* mGameLog;
#if !OGRE_THREAD_PROVIDER
    std::mutex mLogLockMutex;
#endif
};

#endif // _LOGSINKOGRE_H_
