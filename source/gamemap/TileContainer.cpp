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

#include "gamemap/TileContainer.h"

const std::vector<Tile*> EMPTY_TILES;

TileContainer::TileContainer():
    mMapSizeX(0),
    mMapSizeY(0),
    mRr(0),
    mTiles(NULL)
{
}

TileContainer::~TileContainer()
{
    clearTiles();
}

void TileContainer::clearTiles()
{
    if (mTiles)
    {
        for (int ii = 0; ii < mMapSizeX; ++ii)
        {
            for (int jj = 0; jj < mMapSizeY; ++jj)
            {
                mTiles[ii][jj]->deleteYourself();
            }
            delete[] mTiles[ii];
        }
        delete[] mTiles;
        mTiles = NULL;
    }
}

bool TileContainer::addTile(Tile* t)
{
    if (t->x < getMapSizeX() && t->y < getMapSizeY() && t->x >= 0 && t->y >= 0)
    {
        if(mTiles[t->x][t->y] != NULL)
            mTiles[t->x][t->y]->deleteYourself();
        mTiles[t->x][t->y] = t;
        return true;
    }

    return false;
}

void TileContainer::setTileNeighbors(Tile *t)
{
    for (unsigned int i = 0; i < 2; ++i)
    {
        int tempX = t->x, tempY = t->y;
        switch (i)
        {
        case 0:
            --tempX;
            break;
        case 1:
            --tempY;
            break;

        default:
            std::cerr << "\n\n\nERROR:  Unknown neighbor index.\n\n\n";
            exit(1);
        }

        // If the current neigbor tile exists, add the current tile as one of its
        // neighbors and add it as one of the current tile's neighbors.
        if (tempX >= 0 && tempY >= 0)
        {
            Tile *tempTile = getTile(tempX, tempY);
            tempTile->addNeighbor(t);
            t->addNeighbor(tempTile);
        }
    }
}

Tile* TileContainer::getTile(int xx, int yy) const
{
    if(mTiles == NULL)
        return NULL;

    if (xx < getMapSizeX() && yy < getMapSizeY() && xx >= 0 && yy >= 0)
        return mTiles[xx][yy];
    else
    {
        // std :: cerr << " invalid x,y coordinates to getTile" << std :: endl;
        return NULL;
    }
}

const Tile::TileType* TileContainer::getNeighborsTypes(Tile* curTile)
{
    static Tile::TileType neighborsType[8];

    int xx = curTile->getX();
    int yy = curTile->getY();

    neighborsType[0] = getSafeTileType(getTile(xx - 1, yy));
    neighborsType[1] = getSafeTileType(getTile(xx - 1, yy + 1));
    neighborsType[2] = getSafeTileType(getTile(xx, yy + 1));
    neighborsType[3] = getSafeTileType(getTile(xx + 1, yy + 1));
    neighborsType[4] = getSafeTileType(getTile(xx + 1, yy));
    neighborsType[5] = getSafeTileType(getTile(xx + 1, yy - 1));
    neighborsType[6] = getSafeTileType(getTile(xx, yy - 1));
    neighborsType[7] = getSafeTileType(getTile(xx - 1, yy-1));

    return const_cast<Tile::TileType*>(neighborsType);
}

const bool* TileContainer::getNeighborsFullness(Tile* curTile)
{
    static bool neighborsFullness[8];

    int xx = curTile->getX();
    int yy = curTile->getY();

    neighborsFullness[0] = getSafeTileFullness(getTile(xx - 1, yy));
    neighborsFullness[1] = getSafeTileFullness(getTile(xx - 1, yy + 1));
    neighborsFullness[2] = getSafeTileFullness(getTile(xx, yy + 1));
    neighborsFullness[3] = getSafeTileFullness(getTile(xx + 1,yy + 1));
    neighborsFullness[4] = getSafeTileFullness(getTile(xx + 1, yy));
    neighborsFullness[5] = getSafeTileFullness(getTile(xx + 1,yy - 1));
    neighborsFullness[6] = getSafeTileFullness(getTile(xx, yy - 1));
    neighborsFullness[7] = getSafeTileFullness(getTile(xx - 1,yy - 1));

    return const_cast<bool*>(neighborsFullness);
}

unsigned int TileContainer::numTiles()
{
    return mMapSizeX * mMapSizeY;
}

bool TileContainer::allocateMapMemory(int xSize, int ySize)
{
    if (xSize <= 0 || ySize <= 0)
    {
        std::cerr << "Invalid map size given. Couldn't allocate map memory" << std::endl;
        return false;
    }

    // Clear memory usage first
    if (mTiles)
    {
        for(int ii = 0; ii < mMapSizeX; ++ii)
        {
            for(int jj = 0; jj < mMapSizeY; ++jj)
            {
                delete  mTiles[ii][jj];
            }
            delete [] mTiles[ii];
        }
        delete [] mTiles;
    }

    // Set map size
    mMapSizeX = xSize;
    mMapSizeY = ySize;

    mTiles = new Tile **[mMapSizeX];
    if(!mTiles)
    {
        std::cerr << "Failed to allocate map memory" << std::endl;
        return false;
    }
    for(int ii = 0; ii < mMapSizeX; ++ii)
    {
        mTiles[ii] = new Tile *[mMapSizeY];
        for(int jj = 0; jj < mMapSizeY; ++jj)
        {
            mTiles[ii][jj] = NULL;
        }
    }

    return true;
}

Tile::TileType TileContainer::getSafeTileType(Tile* tt)
{
    return (tt == NULL) ? Tile::nullTileType : tt->getType();
}

bool  TileContainer::getSafeTileFullness(Tile* tt)
{
    return (tt == NULL) ? false : (tt->getFullness() > 0 );
}

std::vector<Tile*> TileContainer::rectangularRegion(int x1, int y1, int x2, int y2)
{
    std::vector<Tile*> returnList;
    Tile *tempTile;

    if (x1 > x2)
        std::swap(x1, x2);
    if (y1 > y2)
        std::swap(y1, y2);

    for (int ii = x1; ii <= x2; ++ii)
    {
        for (int jj = y1; jj <= y2; ++jj)
        {
            tempTile = getTile(ii, jj);

            if (tempTile != NULL)
                returnList.push_back(tempTile);
        }
    }

    return returnList;
}

std::vector<Tile*> TileContainer::circularRegion(int x, int y, double radius) const
{
    std::vector<Tile*> returnList;
    Tile *tempTile;
    int xDist, yDist, distSquared;
    double radiusSquared = radius * radius;

    if (radius < 0.0)
        radius = 0.0;

    for (int i = x - radius; i <= x + radius; ++i)
    {
        for (int j = y - radius; j <= y + radius; ++j)
        {
            //TODO:  This routine could be sped up by using the neighborTiles function.
            xDist = i - x;
            yDist = j - y;
            distSquared = xDist * xDist + yDist * yDist;
            if (distSquared < radiusSquared)
            {
                tempTile = getTile(i, j);
                if (tempTile != NULL)
                    returnList.push_back(tempTile);
            }
        }
    }
    return returnList;
}

std::vector<Tile*> TileContainer::tilesBorderedByRegion(const std::vector<Tile*> &region)
{
    std::vector<Tile*> returnList;

    // Loop over all the tiles in the specified region.
    for (Tile* t1 : region)
    {
        // Get the tiles bordering the current tile and loop over them.
        for (Tile* t2 : t1->getAllNeighbors())
        {
            // We add the tile in the return list if it is not already there or in the region
            if((std::find(region.begin(), region.end(), t2) == region.end()) &&
               (std::find(returnList.begin(), returnList.end(), t2) == returnList.end()))
            {
                returnList.push_back(t2);
            }
        }
    }

    return returnList;
}

const std::vector<Tile*>& TileContainer::neighborTiles(int x, int y) const
{
    Tile *tempTile = getTile(x, y);
    if (tempTile == NULL)
        return EMPTY_TILES;

    return tempTile->getAllNeighbors();
}
