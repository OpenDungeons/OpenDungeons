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

#ifndef TESTMODEMANAGER_H
#define TESTMODEMANAGER_H

#include "modes/AbstractModeManager.h"

class TestModeManager: public AbstractModeManager
{
public:
    TestModeManager(ModeType mode)
        : mode(mode)
    {
    }

    ModeType getCurrentModeTypeExceptConsole() const
    {
        return mode;
    }

    ModeType getCurrentModeType()
    {
        return mode;
    }
private:
    ModeType mode;
};

#endif // TESTMODEMANAGER_H
