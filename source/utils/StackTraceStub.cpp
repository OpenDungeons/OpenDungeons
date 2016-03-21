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

#include "utils/StackTracePrint.h"

#include <SFML/System.hpp>

class StackTracePrintPrivateData
{
public:
    StackTracePrintPrivateData()
    {
    }
};

static sf::Mutex gMutex;

StackTracePrintPrivateData* StackTracePrint::mPrivateData = nullptr;

StackTracePrint::StackTracePrint(const std::string& crashFilePath)
{
    sf::Lock lock(gMutex);

    if(mPrivateData != nullptr)
        throw std::exception();

    mPrivateData = new StackTracePrintPrivateData;
}

StackTracePrint::~StackTracePrint()
{
    if(mPrivateData != nullptr)
    {
        delete mPrivateData;
        mPrivateData = nullptr;
    }
}
