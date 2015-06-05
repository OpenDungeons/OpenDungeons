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

#include "LogManagerFile.h"

#include "utils/ResourceManager.h"

#include <SFML/System.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>

static sf::Mutex gMutex;

LogManagerFile::LogManagerFile(const std::string& userDataPath) :
    mLevel(LogMessageLevel::NORMAL)
{
    mFileLog.open(userDataPath);
}

void LogManagerFile::logMessage(const std::string& message, LogMessageLevel lml)
{
    if(mLevel > lml)
        return;

    static std::locale loc(std::wcout.getloc(),
        new boost::posix_time::time_facet("%Y%m%d_%H%M%S"));

    std::stringstream ss;
    ss.imbue(loc);
    ss << "[" << boost::posix_time::second_clock::local_time() << "] " << message << "\n";

    sf::Lock lock(gMutex);
    mFileLog << ss.str();
    mFileLog.flush();
    std::cout << ss.str();
}

void LogManagerFile::setLogDetail(LogMessageLevel ll)
{
    mLevel = ll;
}
