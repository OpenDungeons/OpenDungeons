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

#include "LogManagerOgre.h"

#include "utils/ResourceManager.h"

#include <OgreLogManager.h>
#include <boost/date_time/posix_time/posix_time.hpp>

Ogre::LoggingLevel toOgreLl(LogMessageLevel ll)
{
    //Since ogre also use 1-2-3 as log levels, we can simply cast the level.
    //NOTE: we subtract from 4 as LogMessageLevel is the inverse the corresponding logLevel.
    return static_cast<Ogre::LoggingLevel>(4 - static_cast<int>(ll));
}

Ogre::LogMessageLevel toOgreLml(LogMessageLevel lml)
{
    return static_cast<Ogre::LogMessageLevel>(lml);
}

LogManagerOgre::LogManagerOgre(const std::string& userDataPath)
{
    mLogManager = new Ogre::LogManager;
#ifdef LOGMANAGER_USE_LOCKS
    /* Using a separate log if ogre doesn't have thread support as
     * as log writes from ogre itself won't be thread-safe in this case.
     */
    mGameLog = mLogManager->createLog(userDataPath + GAMELOG_NAME);
#else
    mGameLog = mLogManager->createLog(userDataPath, true, true, false);
#endif
    std::cout << "Log created file=" << userDataPath << std::endl;
}

LogManagerOgre::~LogManagerOgre()
{
    if(mLogManager != nullptr)
        delete mLogManager;
}

void LogManagerOgre::logMessage(const std::string& message, LogMessageLevel lml)
{
#ifdef LOGMANAGER_USE_LOCKS
    std::lock_guard<std::mutex> lock(mLogLockMutex);
#endif
    mGameLog->logMessage(message, toOgreLml(lml));
}

void LogManagerOgre::setLogDetail(LogMessageLevel ll)
{
#ifdef LOGMANAGER_USE_LOCKS
    std::lock_guard<std::mutex> lock(mLogLockMutex);
#endif
    mGameLog->setLogDetail(toOgreLl(ll));
}
