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

#ifndef ODCONSOLECOMMAND_H
#define ODCONSOLECOMMAND_H

#include "GameMap.h"

class ODConsoleCommand
{
public:
    virtual void execute(GameMap* gameMap) = 0;
};

class ODConsoleCommandAddGold : public ODConsoleCommand
{
public:
    ODConsoleCommandAddGold(int gold, int color) :
        mGold(gold),
        mColor(color)
    {
    }

protected:
    virtual void execute(GameMap* gameMap)
    {
        gameMap->addGoldToSeat(mGold, mColor);
    }

private:
    int mGold;
    int mColor;
};

#endif // ODCONSOLECOMMAND_H
