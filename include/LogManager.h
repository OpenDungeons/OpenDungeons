/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  <copyright holder> <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#include <semaphore.h>
#endif

/*! \brief Helper/wapper class to provide thread-safe logging when ogre is compiled without threads.
 *
 */
class LogManager : public Ogre::Singleton<LogManager>
{

public:
    LogManager();
    ~LogManager();

    void logMessage(const std::string& message, Ogre::LogMessageLevel lml = Ogre::LML_NORMAL,
                    bool maskDebug = false);
    void setLogDetail(Ogre::LoggingLevel ll);
    Ogre::LoggingLevel getLogDetail();
private:
    LogManager(const LogManager& other);
    LogManager& operator=(const LogManager& other);
    Ogre::Log* gameLog;
    static const std::string GAMELOG_NAME;
#ifdef LOGMANAGER_USE_LOCKS
    sem_t logLockSemaphore;
#endif
};

#endif // LOGMANAGER_H
