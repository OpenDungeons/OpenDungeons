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

#include "utils/LogManager.h"

template<> LogManager* Ogre::Singleton<LogManager>::msSingleton = nullptr;

//! \brief Log filename used when OD Application throws errors without using Ogre default logger.
const std::string LogManager::GAMELOG_NAME = "gameLog";

LogManager::LogManager()
{

}

LogManager::~LogManager()
{

}

void LogManager::addSink(const std::shared_ptr<LogSink>& sink)
{
    mSinks.push_back(sink);
}

void LogManager::logMessage(LogMessageLevel level, const char* module, const char* filepath, int line, const char* format, ...)
{
    std::unique_lock<std::mutex>(mLock);

    // timestamp

    time_t current_time = ::time(0);
    struct tm* now = ::localtime(&current_time);

    char timestamp[32] = { 0 };
    snprintf(
        timestamp,
        31,
        "%02d:%02d:%02d",
        now->tm_hour,
        now->tm_min,
        now->tm_sec);

    // filename

#if WIN32 || _WINDOWS
    const char* path_last_slash = strrchr(filepath, '\\');
#else
    const char* path_last_slash = strrchr(filepath, '/');
#endif
    const char* filename = (path_last_slash != nullptr) ? (path_last_slash + 1) : filepath;

    // message

    char message_formatted[1024] = { 0 };

    va_list arguments;
    va_start(arguments, format);
    vsprintf(message_formatted, format, arguments);
    va_end(arguments);

    for (const auto& sink : mSinks)
    {
        sink->write(level, module, timestamp, filename, line, message_formatted);
    }
}