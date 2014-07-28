/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#include "LogManager.h"

#include "ResourceManager.h"

#include <OgreLogManager.h>

#include "boost/date_time/posix_time/posix_time.hpp"

template<> LogManager* Ogre::Singleton<LogManager>::msSingleton = 0;

//! \brief Log filename used when OD Application throws errors without using Ogre default logger.
const std::string LogManager::GAMELOG_NAME = "gameLog";

LogManager::LogManager()
{
#ifdef LOGMANAGER_USE_LOCKS
    /* Using a separate log if ogre doesn't have thread support as
     * as log writes from ogre itself won't be thread-safe in this case.
     */
    mGameLog = Ogre::LogManager::getSingleton().createLog(
        ResourceManager::getSingleton().getHomePath() + GAMELOG_NAME);
    sem_init(&logLockSemaphore, 0, 1);
#else
    mGameLog = Ogre::LogManager::getSingleton().getDefaultLog();
#endif
}

void LogManager::logMessage(const std::string& message, Ogre::LogMessageLevel lml,
                            bool maskDebug, bool addTimeStamp)
{
#ifdef LOGMANAGER_USE_LOCKS
    sem_wait(&logLockSemaphore);
#endif
    if(addTimeStamp)
    {
        static std::locale loc(std::wcout.getloc(),
            new boost::posix_time::wtime_facet(L"%Y%m%d_%H%M%S"));

        std::stringstream ss;
        ss.imbue(loc);
        ss << "[" << boost::posix_time::second_clock::universal_time() << "] " << message;
        mGameLog->logMessage(ss.str(), lml, maskDebug);
    }
    else
    {
        mGameLog->logMessage(message, lml, maskDebug);
    }
#ifdef LOGMANAGER_USE_LOCKS
    sem_post(&logLockSemaphore);
#endif
}

void LogManager::setLogDetail(Ogre::LoggingLevel ll)
{
#ifdef LOGMANAGER_USE_LOCKS
    sem_wait(&logLockSemaphore);
#endif
    mGameLog->setLogDetail(ll);
#ifdef LOGMANAGER_USE_LOCKS
    sem_post(&logLockSemaphore);
#endif
}

Ogre::LoggingLevel LogManager::getLogDetail()
{
    Ogre::LoggingLevel ret;
#ifdef LOGMANAGER_USE_LOCKS
    sem_wait(&logLockSemaphore);
#endif
    ret = mGameLog->getLogDetail();
#ifdef LOGMANAGER_USE_LOCKS
    sem_post(&logLockSemaphore);
#endif
    return ret;
}
