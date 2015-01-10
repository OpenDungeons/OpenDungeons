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

#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <string>

#include <OgreSingleton.h>
#include <OgreConfig.h>
#include <OgreLog.h>

//If ogre is not build with thread support we need to lock the access ourselves.
#if OGRE_THREAD_PROVIDER == 0
#define LOGMANAGER_USE_LOCKS
#endif

#ifdef LOGMANAGER_USE_LOCKS
#include <mutex>
#endif

#define OD_STRING_S(x) #x
#define OD_STRING_S_(x) OD_STRING_S(x)
#define S__LINE__ OD_STRING_S_(__LINE__)
#define OD_ASSERT_TRUE(a)        if(!(a)) LogManager::getSingleton().logMessage("ERROR: Assert failed file " + std::string(__FILE__) + " line " + std::string(S__LINE__))
#define OD_ASSERT_TRUE_MSG(a,b)  if(!(a)) LogManager::getSingleton().logMessage("ERROR: Assert failed file " + std::string(__FILE__) + " line " + std::string(S__LINE__) + std::string(" info : ") + b)


//! \brief Helper/wrapper class to provide thread-safe logging when ogre is compiled without threads.
class LogManager : public Ogre::Singleton<LogManager>
{
public:
    LogManager();

    ~LogManager()
    {}

    //! \brief Log a message to the game log.
    void logMessage(const std::string& message, Ogre::LogMessageLevel lml = Ogre::LML_NORMAL,
                    bool maskDebug = false, bool addTimeStamp = false);

    //! \brief Set the log detail level.
    void setLogDetail(Ogre::LoggingLevel ll);

    Ogre::Log& getLog()
    { return *mGameLog; }

    Ogre::LoggingLevel getLogDetail();

    static const std::string GAMELOG_NAME;

private:
    Ogre::Log* mGameLog;
#ifdef LOGMANAGER_USE_LOCKS
    std::mutex mLogLockMutex;
#endif
};

#endif // LOGMANAGER_H
