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

#ifndef LOGMANAGEROGRE_H
#define LOGMANAGEROGRE_H

#include "utils/LogManager.h"

#include <OgreConfig.h>

//If ogre is not build with thread support we need to lock the access ourselves.
#if OGRE_THREAD_PROVIDER == 0
#define LOGMANAGER_USE_LOCKS
#endif

#ifdef LOGMANAGER_USE_LOCKS
#include <mutex>
#endif

namespace Ogre
{
    class Log;
}

//! \brief Logger implementation using the logging system from ogre
class LogManagerOgre final : public LogManager
{
public:
    LogManagerOgre(const std::string& userDataPath);
    void logMessage(const std::string& message, LogMessageLevel lml = LogMessageLevel::NORMAL,
                    bool maskDebug = false, bool addTimeStamp = false) override;
    void setLogDetail(LogMessageLevel ll) override;
private:
    Ogre::Log* mGameLog;
#ifdef LOGMANAGER_USE_LOCKS
    std::mutex mLogLockMutex;
#endif
};

#endif // LOGMANAGEROGRE_H
