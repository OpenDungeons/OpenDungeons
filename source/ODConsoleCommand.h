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
    ODConsoleCommand() {}
    virtual ~ODConsoleCommand() {}
    virtual void execute(GameMap* gameMap) = 0;
};

class ODConsoleCommandAddGold : public ODConsoleCommand
{
public:
    ODConsoleCommandAddGold(int gold, int seatId) :
        mGold(gold),
        mSeatId(seatId)
    {
    }

protected:
    virtual void execute(GameMap* gameMap)
    {
        gameMap->addGoldToSeat(mGold, mSeatId);
    }

private:
    int mGold;
    int mSeatId;
};

class ODConsoleCommandLogFloodFill : public ODConsoleCommand
{
public:
    ODConsoleCommandLogFloodFill()
    {
    }

protected:
    virtual void execute(GameMap* gameMap)
    {
        gameMap->logFloodFileTiles();
    }
};

class ODConsoleCommandSetCreatureDestination : public ODConsoleCommand
{
public:
    ODConsoleCommandSetCreatureDestination(const std::string& creatureName, int x, int y):
        mCreatureName(creatureName),
        mX(x),
        mY(y)
    {
    }

protected:
    virtual void execute(GameMap* gameMap)
    {
        gameMap->consoleSetCreatureDestination(mCreatureName, mX, mY);
    }
private:
    std::string mCreatureName;
    int mX;
    int mY;
};

#endif // ODCONSOLECOMMAND_H
