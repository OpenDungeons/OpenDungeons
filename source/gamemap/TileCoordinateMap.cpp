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

#include "gamemap/TileCoordinateMap.h"

#include <algorithm>

TileCoordinateMap::TileCoordinateMap(const int nRadius) :
    mRadius(nRadius)
{
    precomputeMap(nRadius);
}

void TileCoordinateMap::precomputeMap(const int sightRadius)
{
    mData.clear();

    //TODO: This loop can be made to list the visible region in a spiral pattern
    // so that all of the tiles appear in the tileQueue already sorted.
    int sightRadiusSquared = sightRadius * sightRadius;
    for (int i = -1 * sightRadius; i <= sightRadius; ++i)
    {
        for (int j = -1 * sightRadius; j <= sightRadius; ++j)
        {
            int rSquared = i * i + j * j;
            if (rSquared > sightRadiusSquared)
                continue;

            mData.push_back(TileCoordinateData(RadialVector2(i, j), rSquared,
                            std::pair<int, int>(i, j)));
        }
    }

    // Sort the tile queue so that if we start at any point in the tile queue
    // and iterate forward from that point, every successive tile will be as far away from,
    // or farther away from the target tile point.
    std::sort(mData.begin(), mData.end(), TileCoordinateMap::dataSortComparitor);
}

std::pair<int, int> TileCoordinateMap::getCoordinate(const int i)
{
    checkIndex(i);
    return mData[i].getCoord();
}

double TileCoordinateMap::getCentralTheta(const int i)
{
    checkIndex(i);
    return mData[i].getVec().getTheta();
}

int TileCoordinateMap::getRadiusSquared(const int i)
{
    checkIndex(i);
    return mData[i].getRadiusSquared();
}

bool TileCoordinateMap::dataSortComparitor(TileCoordinateData t1, TileCoordinateData t2)
{
    return t1.getRadiusSquared() < t2.getRadiusSquared();
}

void TileCoordinateMap::checkIndex(const unsigned int i)
{
    if (i >= mData.size())
    {
        int newRadius = mRadius * 2;
        precomputeMap(newRadius);
        mRadius = newRadius;
    }
}
