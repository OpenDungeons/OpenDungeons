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

#include "DirectionalTrap.h"

#include "Tile.h"
#include "GameMap.h"

DirectionalTrap::DirectionalTrap(int xdir, int ydir):
    Trap()
{
    mDir = std::pair<int, int>(xdir, ydir);
}

std::pair<int, int> DirectionalTrap::projectionOnBorder(int xdir, int ydir)
{
    GameMap* gm = getGameMap();
    int a = (ydir - mCoveredTiles[0]->y) / (xdir - mCoveredTiles[0]->x);
    int b = ydir - a * xdir;

    if(b >= 0 && b < gm->getMapSizeY())
    {
        return std::pair<int, int>(0, b);
    }
    else if(-(b/a) >= 0 && -(b/a) < gm->getMapSizeX())
    {
        return std::pair<int, int>((-b)/a, 0);
    }

    int tmp = a * (gm->getMapSizeY() - 1) + b;
    if(tmp >= 0 && tmp < gm->getMapSizeY())
        return std::pair<int, int>(gm->getMapSizeX() - 1, tmp);

    return std::pair<int, int>((gm->getMapSizeY() - b - 1)/a, gm->getMapSizeY() - 1);
}
