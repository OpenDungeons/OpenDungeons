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

#ifndef SERVERCONSOLECOMMANDS_H
#define SERVERCONSOLECOMMANDS_H

#include "gamemap/GameMap.h"
#include "entities/Creature.h"

class ServerConsoleCommand
{
public:
    ServerConsoleCommand() {}
    virtual ~ServerConsoleCommand() {}
    virtual void execute(GameMap* gameMap) = 0;
};

class SCCAddGold : public ServerConsoleCommand
{
public:
    SCCAddGold(int gold, int seatId) :
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

class SCCLogFloodFill : public ServerConsoleCommand
{
public:
    SCCLogFloodFill()
    {
    }

protected:
    virtual void execute(GameMap* gameMap)
    {
        gameMap->logFloodFileTiles();
    }
};

class SCCSetCreatureDestination : public ServerConsoleCommand
{
public:
    SCCSetCreatureDestination(const std::string& creatureName, int x, int y):
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

class SCCDisplayCreatureVisualDebug : public ServerConsoleCommand
{
public:
    SCCDisplayCreatureVisualDebug(const std::string& creatureName):
        mCreatureName(creatureName)
    {
    }

protected:
    virtual void execute(GameMap* gameMap)
    {
        gameMap->consoleToggleCreatureVisualDebug(mCreatureName);
    }
private:
    std::string mCreatureName;
};

class SCCDisplaySeatVisualDebug : public ServerConsoleCommand
{
public:
    SCCDisplaySeatVisualDebug(int seatId, bool enable):
        mSeatId(seatId),
        mEnable(enable)
    {
    }

protected:
    virtual void execute(GameMap* gameMap)
    {
        gameMap->consoleDisplaySeatVisualDebug(mSeatId, mEnable);
    }
private:
    int mSeatId;
    bool mEnable;
};

class SCCSetLevelCreature : public ServerConsoleCommand
{
public:
    SCCSetLevelCreature(const std::string& creatureName, uint32_t level):
        mCreatureName(creatureName),
        mLevel(level)
    {
    }

protected:
    virtual void execute(GameMap* gameMap)
    {
        gameMap->consoleSetLevelCreature(mCreatureName, mLevel);
    }
private:
    std::string mCreatureName;
    uint32_t mLevel;
};

class SCCAskToggleFOW : public ServerConsoleCommand
{
public:
    SCCAskToggleFOW()
    {
    }

protected:
    virtual void execute(GameMap* gameMap)
    {
        gameMap->consoleAskToggleFOW();
    }
};

class SCCAddCreature : public ServerConsoleCommand
{
public:
    SCCAddCreature(const std::string& arguments):
        mArguments(arguments)
    {
    }

protected:
    virtual void execute(GameMap* gameMap)
    {
        // Creature the creature and add it to the gameMap
        std::stringstream stringStr(mArguments);
        Creature* creature = Creature::getCreatureFromStream(gameMap, stringStr);

        creature->createMesh();
        creature->addToGameMap();
    }

private:
    std::string mArguments;
};

#endif // SERVERCONSOLECOMMANDS_H
