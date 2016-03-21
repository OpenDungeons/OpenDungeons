/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#include <memory>
#include <mutex>
#include <string>

#include <SFML/System.hpp>

#include <OgreSingleton.h>

#include "utils/Helper.h"
#include "utils/LogMessageLevel.h"
#include "utils/LogSink.h"

#define OD_LOG_ERR(_message)                      LogManager::getSingleton().logMessage(LogMessageLevel::CRITICAL, __FILE__, __LINE__, (std::string("") + _message))
#define OD_LOG_WRN(_message)                      LogManager::getSingleton().logMessage(LogMessageLevel::WARNING, __FILE__, __LINE__, (std::string("") + _message))
#define OD_LOG_INF(_message)                      LogManager::getSingleton().logMessage(LogMessageLevel::NORMAL, __FILE__, __LINE__, (std::string("") + _message))
#define OD_LOG_DBG(_message)                      LogManager::getSingleton().logMessage(LogMessageLevel::TRIVIAL, __FILE__, __LINE__, (std::string("") + _message))

#define OD_ASSERT_TRUE(_condition)                if (!(_condition)) LogManager::getSingleton().logMessage(LogMessageLevel::CRITICAL, __FILE__, __LINE__, std::string(#_condition))
#define OD_ASSERT_TRUE_MSG(_condition, _message)  if (!(_condition)) LogManager::getSingleton().logMessage(LogMessageLevel::CRITICAL, __FILE__, __LINE__, (std::string("") + _message))

//! \brief Helper/wrapper class to provide thread-safe logging when ogre is compiled without threads.
class LogManager : public Ogre::Singleton<LogManager>
{
public:
    LogManager();
    ~LogManager();

    //! \brief Add a sink for log messages.
    void addSink(std::unique_ptr<LogSink> sink);

    //! \brief Set the global minimum logging level.
    void setLevel(LogMessageLevel level);

    //! \brief Set the minimum logging level per module.
    void setModuleLevel(const char* module, LogMessageLevel level);

    //! \brief Log a message to the sinks.
    void logMessage(LogMessageLevel level, const char* filepath, int line, const std::string& message);

    static const std::string GAMELOG_NAME;
private:
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;

    LogMessageLevel mLevel;
    std::map<std::string, LogMessageLevel> mModuleLevel;
    sf::Mutex mLock;
    std::vector<std::unique_ptr<LogSink>> mSinks;
    std::stringstream mTimestampStream;
};

#endif // LOGMANAGER_H
