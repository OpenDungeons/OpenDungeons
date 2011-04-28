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

#include <OgreLogManager.h>

#include "ResourceManager.h"

#include "LogManager.h"

template<> LogManager*
        Ogre::Singleton<LogManager>::ms_Singleton = 0;

/*! \brief Returns a reference to the singleton object of LogManager.
 *
 */
LogManager& LogManager::getSingleton()
{
    assert(ms_Singleton);
    return (*ms_Singleton);
}

/*! \brief Returns a pointer to the singleton object of LogManager.
 *
 */
LogManager* LogManager::getSingletonPtr()
{
    return ms_Singleton;
}

LogManager::LogManager()
{
#ifdef LOGMANAGER_USE_LOCKS
/*Using a separate log if ogre doesn't have thread support as
 *as log writes from ogre itself won't be thread-safe in this case.
 */
    gameLog = Ogre::LogManager::getSingleton().createLog(
        ResourceManager::getSingleton().getHomePath() + GAMELOG_NAME);
    sem_init(&logLockSemaphore, 0, 1);
#else
    gameLog = Ogre::LogManager::getSingleton().getDefaultLog();
#endif
}

LogManager::~LogManager()
{

}

/*! \brief Log a message to the game log.
 *
 */
void LogManager::logMessage(const std::string& message, Ogre::LogMessageLevel lml,
                    bool maskDebug)
{
#ifdef LOGMANAGER_USE_LOCKS
    sem_wait(&logLockSemaphore);
#endif
    gameLog->logMessage(message, lml, maskDebug);
#ifdef LOGMANAGER_USE_LOCKS
    sem_post(&logLockSemaphore);
#endif
}

/*! \brief Set the log detail level.
 *
 */
void LogManager::setLogDetail(Ogre::LoggingLevel ll)
{
#ifdef LOGMANAGER_USE_LOCKS
    sem_wait(&logLockSemaphore);
#endif
    gameLog->setLogDetail(ll);
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
    ret = gameLog->getLogDetail();
#ifdef LOGMANAGER_USE_LOCKS
    sem_post(&logLockSemaphore);
#endif
    return ret;
}

const std::string LogManager::GAMELOG_NAME = "gameLog";