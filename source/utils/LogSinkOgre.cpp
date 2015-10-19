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

#include "utils/LogSinkOgre.h"

#include "utils/LogManager.h"

#include <OgreLog.h>
#include <OgreLogManager.h>

Ogre::LoggingLevel toOgreLl(LogMessageLevel level)
{
    switch (level)
    {
    case LogMessageLevel::TRIVIAL:
        return Ogre::LL_LOW;

    case LogMessageLevel::NORMAL:
        return Ogre::LL_NORMAL;

    case LogMessageLevel::WARNING:
    case LogMessageLevel::CRITICAL:
        return Ogre::LL_BOREME;

    default:
        OD_ASSERT_TRUE_MSG(0, "Unhandled LogMessageLevel %d.");
        return Ogre::LL_LOW;

    }
}

Ogre::LogMessageLevel toOgreLml(LogMessageLevel level)
{
    switch (level)
    {
    case LogMessageLevel::TRIVIAL:
        return Ogre::LML_TRIVIAL;

    case LogMessageLevel::NORMAL:
    case LogMessageLevel::WARNING:
        return Ogre::LML_NORMAL;

    case LogMessageLevel::CRITICAL:
        return Ogre::LML_CRITICAL;

    default:
        OD_ASSERT_TRUE_MSG(0, "Unhandled LogMessageLevel %d.");
        return Ogre::LML_TRIVIAL;

    }
}

LogSinkOgre::LogSinkOgre(const std::string& userDataPath)
    : mLogManager(new Ogre::LogManager)
{
#if !OGRE_THREAD_PROVIDER
    /* Using a separate log if ogre doesn't have thread support as
    * as log writes from ogre itself won't be thread-safe in this case.
    */
    mGameLog = mLogManager->createLog(userDataPath + GAMELOG_NAME);
#else
    mGameLog = mLogManager->createLog(userDataPath, true, true, false);
#endif
}

LogSinkOgre::~LogSinkOgre()
{

}

void LogSinkOgre::write(LogMessageLevel level, const char* module, const char* timestamp, const char* filename, int line, const char* message)
{
#if !OGRE_THREAD_PROVIDER
    std::lock_guard<std::mutex> lock(mLogLockMutex);
#endif
    mGameLog->logMessage(Ogre::String(message), toOgreLml(level));
}