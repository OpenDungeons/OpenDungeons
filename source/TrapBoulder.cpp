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

#include "TrapBoulder.h"
#include "ODPacket.h"
#include "GameMap.h"
#include "LogManager.h"

TrapBoulder::TrapBoulder(GameMap* gameMap, int x, int y) :
    DirectionalTrap(gameMap, x, y)
{
    mReloadTime = 20;
    mMinDamage = 30;
    mMaxDamage = 40;
    setMeshName("Boulder");
}

TrapBoulder::TrapBoulder(GameMap* gameMap) :
    DirectionalTrap(gameMap)
{
    mReloadTime = 20;
    mMinDamage = 30;
    mMaxDamage = 40;
    setMeshName("Boulder");
}

TrapBoulder* TrapBoulder::getTrapBoulderFromStream(GameMap* gameMap, std::istream& is)
{
    TrapBoulder* trap = new TrapBoulder(gameMap);
    is >> trap;
    return trap;
}

TrapBoulder* TrapBoulder::getTrapBoulderFromPacket(GameMap* gameMap, ODPacket& is)
{
    TrapBoulder* trap = new TrapBoulder(gameMap);
    is >> trap;
    return trap;
}

std::istream& operator>>(std::istream& is, TrapBoulder *trap)
{
    int tilesToLoad, tempX, tempY, tempInt;

    is >> tempInt;
    trap->setSeat(trap->getGameMap()->getSeatById(tempInt));

    is >> tilesToLoad;
    for (int i = 0; i < tilesToLoad; ++i)
    {
        is >> tempX >> tempY;
        Tile *tempTile = trap->getGameMap()->getTile(tempX, tempY);
        if (tempTile != NULL)
        {
            trap->addCoveredTile(tempTile, Trap::DEFAULT_TILE_HP);
            tempTile->setSeat(trap->getSeat());
        }
    }
    is >> tempX >> tempY;
    trap->mDir = std::pair<int, int>(tempX, tempY);

    return is;
}

std::ostream& operator<<(std::ostream& os, TrapBoulder *trap)
{
    int32_t nbTiles = trap->mCoveredTiles.size();
    int seatId = trap->getSeat()->getId();
    os << seatId << "\t" << nbTiles << "\n";
    for(std::vector<Tile*>::iterator it = trap->mCoveredTiles.begin(); it != trap->mCoveredTiles.end(); ++it)
    {
        Tile *tempTile = *it;
        os << tempTile->x << "\t" << tempTile->y << "\n";
    }
    os << trap->mDir.first << "\t" << trap->mDir.second << "\n";
    return os;
}

ODPacket& operator>>(ODPacket& is, TrapBoulder *trap)
{
    int tilesToLoad, tempX, tempY, tempInt;
    std::string name;
    is >> name;
    trap->setName(name);

    is >> tempInt;
    trap->setSeat(trap->getGameMap()->getSeatById(tempInt));

    is >> tilesToLoad;
    for (int i = 0; i < tilesToLoad; ++i)
    {
        is >> tempX >> tempY;
        Tile *tempTile = trap->getGameMap()->getTile(tempX, tempY);
        if (tempTile != NULL)
        {
            trap->addCoveredTile(tempTile, Trap::DEFAULT_TILE_HP);
            tempTile->setSeat(trap->getSeat());
        }
        else
        {
            LogManager::getSingleton().logMessage("ERROR : trying to add trap on unkown tile "
                + Ogre::StringConverter::toString(tempX) + "," + Ogre::StringConverter::toString(tempY));
        }
    }
    is >> tempX >> tempY;
    trap->mDir = std::pair<int, int>(tempX, tempY);
    return is;
}

ODPacket& operator<<(ODPacket& os, TrapBoulder *trap)
{
    int nbTiles = trap->mCoveredTiles.size();
    const std::string& name = trap->getName();
    int seatId = trap->getSeat()->getId();
    os << name << seatId;
    os << nbTiles;
    for(std::vector<Tile*>::iterator it = trap->mCoveredTiles.begin(); it != trap->mCoveredTiles.end(); ++it)
    {
        Tile *tempTile = *it;
        os << tempTile->x << tempTile->y;
    }
    os << trap->mDir.first << trap->mDir.second;

    return os;
}
