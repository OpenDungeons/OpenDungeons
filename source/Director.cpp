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

#include "Director.h"

template<> Director* Ogre::Singleton<Director>::msSingleton = NULL;

Director::Director():
    mIsServer(false)
{
}

Director::~Director()
{
}

int Director::playNextScenario()
{
    return 0;
}

int Director::playScenario(int ss)
{
    return 0;
}

int Director::addScenario(const std::string& scenarioFileName)
{
    return 0;
}

int Director::addScenario(const std::string& scenarioFileName, int ss)
{
    return 0;
}

int Director::removeScenario()
{
    return 0;
}

int Director::removeScenario(int ss)
{
    return 0;
}

int Director::clearScenarios()
{
    return 0;
}
