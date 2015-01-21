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

#include "gamemap/TileContainer.h"

#include "network/ODPacket.h"

#include "utils/LogManager.h"

const std::vector<Tile*> EMPTY_TILES;

class TileDistance
{
public:
    enum TileDistanceType
    {
        Horizontal,
        Diagonal,
        Other
    };

    TileDistance(int diffX, int diffY, TileDistanceType type, int distSquared):
        mDiffX(diffX),
        mDiffY(diffY),
        mType(type),
        mDistSquared(distSquared)
    {
    }

    inline int getDiffX() const
    { return mDiffX; }

    inline int getDiffY() const
    { return mDiffY; }

    inline TileDistanceType getType() const
    { return mType; }

    inline int getDistSquared() const
    { return mDistSquared; }

private:
    int mDiffX;
    int mDiffY;
    TileDistanceType mType;
    int mDistSquared;
};

TileContainer::TileContainer():
    mMapSizeX(0),
    mMapSizeY(0),
    mRr(0),
    mTiles(nullptr),
    mTileDistanceComputed(0)
{
    buildTileDistance(15);
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
        mTiles = nullptr;
    }
}

bool TileContainer::addTile(Tile* t)
{
    int x = t->getX();
    int y = t->getY();

    if (x < getMapSizeX() && y < getMapSizeY() && x >= 0 && y >= 0)
    {
        if(mTiles[x][y] != nullptr)
            mTiles[x][y]->deleteYourself();
        mTiles[x][y] = t;
        return true;
    }

    return false;
}

void TileContainer::setTileNeighbors(Tile *t)
{
    for (unsigned int i = 0; i < 2; ++i)
    {
        int tempX = t->getX(), tempY = t->getY();
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
    if(mTiles == nullptr)
        return nullptr;

    if (xx < getMapSizeX() && yy < getMapSizeY() && xx >= 0 && yy >= 0)
        return mTiles[xx][yy];
    else
    {
        // std :: cerr << " invalid x,y coordinates to getTile" << std :: endl;
        return nullptr;
    }
}

void TileContainer::tileToPacket(ODPacket& packet, Tile* tile) const
{
    int32_t x = tile->getX();
    int32_t y = tile->getY();
    packet << x << y;
}

Tile* TileContainer::tileFromPacket(ODPacket& packet) const
{
    int32_t x;
    int32_t y;
    OD_ASSERT_TRUE(packet >> x >> y);
    return getTile(x, y);
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
            mTiles[ii][jj] = nullptr;
        }
    }

    return true;
}

Tile::TileType TileContainer::getSafeTileType(Tile* tt)
{
    return (tt == nullptr) ? Tile::nullTileType : tt->getType();
}

bool  TileContainer::getSafeTileFullness(Tile* tt)
{
    return (tt == nullptr) ? false : (tt->getFullness() > 0 );
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

            if (tempTile != nullptr)
                returnList.push_back(tempTile);
        }
    }

    return returnList;
}

std::vector<Tile*> TileContainer::circularRegion(int x, int y, int radius)
{
    // To compute the tiles within this region, we use the symmetry of the square. That's why we mix tile x/y coordinate
    // with tileDist diffX/diffY. More explanation can be found in the buildTileDistance function
    std::vector<Tile*> returnList;

    if(radius > mTileDistanceComputed)
        buildTileDistance(radius);

    int radiusSquared = radius * radius;
    for(const TileDistance& tileDist : mTileDistance)
    {
        if(tileDist.getDistSquared() > radiusSquared)
            break;

        switch(tileDist.getType())
        {
            case TileDistance::TileDistanceType::Horizontal:
            {
                // We take the 4 tiles at this distance
                if(tileDist.getDiffX() == 0)
                {
                    // We only add the current tile
                    Tile* tile = getTile(x, y);
                    if(tile != nullptr)
                        returnList.push_back(tile);

                    continue;
                }

                // We add the 4 tiles
                Tile* tile;
                tile = getTile(x + tileDist.getDiffX(), y);
                if(tile != nullptr)
                    returnList.push_back(tile);
                tile = getTile(x - tileDist.getDiffX(), y);
                if(tile != nullptr)
                    returnList.push_back(tile);
                tile = getTile(x, y + tileDist.getDiffX());
                if(tile != nullptr)
                    returnList.push_back(tile);
                tile = getTile(x, y - tileDist.getDiffX());
                if(tile != nullptr)
                    returnList.push_back(tile);

                break;
            }

            case TileDistance::TileDistanceType::Diagonal:
            {
                // We add the 4 tiles
                Tile* tile;
                tile = getTile(x + tileDist.getDiffX(), y + tileDist.getDiffY());
                if(tile != nullptr)
                    returnList.push_back(tile);
                tile = getTile(x + tileDist.getDiffX(), y - tileDist.getDiffY());
                if(tile != nullptr)
                    returnList.push_back(tile);
                tile = getTile(x - tileDist.getDiffX(), y + tileDist.getDiffY());
                if(tile != nullptr)
                    returnList.push_back(tile);
                tile = getTile(x - tileDist.getDiffX(), y - tileDist.getDiffY());
                if(tile != nullptr)
                    returnList.push_back(tile);

                break;
            }

            case TileDistance::TileDistanceType::Other:
            default:
            {
                // We add the 8 tiles
                Tile* tile;
                tile = getTile(x + tileDist.getDiffX(), y + tileDist.getDiffY());
                if(tile != nullptr)
                    returnList.push_back(tile);
                tile = getTile(x + tileDist.getDiffX(), y - tileDist.getDiffY());
                if(tile != nullptr)
                    returnList.push_back(tile);
                tile = getTile(x - tileDist.getDiffX(), y + tileDist.getDiffY());
                if(tile != nullptr)
                    returnList.push_back(tile);
                tile = getTile(x - tileDist.getDiffX(), y - tileDist.getDiffY());
                if(tile != nullptr)
                    returnList.push_back(tile);
                tile = getTile(x + tileDist.getDiffY(), y + tileDist.getDiffX());
                if(tile != nullptr)
                    returnList.push_back(tile);
                tile = getTile(x + tileDist.getDiffY(), y - tileDist.getDiffX());
                if(tile != nullptr)
                    returnList.push_back(tile);
                tile = getTile(x - tileDist.getDiffY(), y + tileDist.getDiffX());
                if(tile != nullptr)
                    returnList.push_back(tile);
                tile = getTile(x - tileDist.getDiffY(), y - tileDist.getDiffX());
                if(tile != nullptr)
                    returnList.push_back(tile);

                break;
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
    if (tempTile == nullptr)
        return EMPTY_TILES;

    return tempTile->getAllNeighbors();
}

void TileContainer::buildTileDistance(int distance)
{
    if(mTileDistanceComputed >= distance)
        return;

    // We want to be able to fill a vector of tiles sorted beginning with the closest tile. If we look a grid (each letter
    // represents a tile at the same distance from the center: a):
    // jihghij
    // ifedefi
    // hecbceh
    // gdbabdg
    // hecbceh
    // ifedefi
    // jihghij
    // We can see that there are 3 kind of tiles:
    // - Vertical/Horizontal tiles (abdg): at each distance, there are 4 of them
    // - Diagonal tiles (acfj): at each distance, there are 4 of them
    // - Other tiles (ehi...): at each distance, there are 8 of them
    // Moreover, we can see a symmetry. We can compute all tiles by computing only 1/8 tiles:
    //    j
    //   fi
    //  ceh
    // abdg

    // If we compute only the minimum tiles needed, we have no vertical tiles (since each of them can be deduced from the horizontal)
    // To compute tiles easily, we will compute the 1/8 tiles until distance. Then, we will sort the tiles to begin with
    // closest distance until farthest
    for(int y = mTileDistanceComputed; y < distance; ++y)
    {
        for(int x = y; x < distance; ++x)
        {
            TileDistance::TileDistanceType type;
            if(y == 0)
            {
                type = TileDistance::TileDistanceType::Horizontal;
            }
            else if(x == y)
            {
                type = TileDistance::TileDistanceType::Diagonal;
            }
            else
            {
                type = TileDistance::TileDistanceType::Other;
            }
            int distSquared = x * x + y * y;
            mTileDistance.push_back(TileDistance(x, y, type, distSquared));
        }
    }

    std::sort(mTileDistance.begin(), mTileDistance.end(), sortByDistSquared);

    mTileDistanceComputed = distance;
}

bool TileContainer::sortByDistSquared(const TileDistance& tileDist1, const TileDistance& tileDist2)
{
    return tileDist1.getDistSquared() < tileDist2.getDistSquared();
}

std::list<Tile*> TileContainer::tilesBetween(int x1, int y1, int x2, int y2)
{
    std::list<Tile*> path;

    double deltax = x2 - x1;
    double deltay = y2 - y1;
    // We don't have to check for deltay == 0 because if deltax > 0 and deltay == 0,
    // we will never have std::abs(deltax) < std::abs(deltay) and, thus, we will
    // never compute std::abs(deltax / deltay);
    if(deltax == 0)
    {
        // Vertical line, no need to compute
        int diffY = 1;
        if(y1 > y2)
            diffY = -1;

        for(int y = y1; y != y2; y += diffY)
        {
            Tile* tile = getTile(x1, y);
            if(tile == nullptr)
                break;

            path.push_back(tile);
        }
    }
    else if(std::abs(deltax) >= std::abs(deltay))
    {
        double error = 0;
        double deltaerr = std::abs(deltay / deltax);
        int diffX = 1;
        if(x1 > x2)
            diffX = -1;

        int diffY = 1;
        if(y1 > y2)
            diffY = -1;

        int y = y1;
        for(int x = x1; x != x2; x += diffX)
        {
            Tile* tile = getTile(x, y);
            if(tile == nullptr)
                break;

            path.push_back(tile);
            error += deltaerr;
            if(error >= 0.5)
            {
                y += diffY;
                error = error - 1.0;
            }
        }
    }
    else // if(std::abs(deltax) < std::abs(deltay))
    {
        double error = 0;
        double deltaerr = std::abs(deltax / deltay);
        int diffX = 1;
        if(x1 > x2)
            diffX = -1;

        int diffY = 1;
        if(y1 > y2)
            diffY = -1;

        int x = x1;
        for(int y = y1; y != y2; y += diffY)
        {
            Tile* tile = getTile(x, y);
            if(tile == nullptr)
                break;

            path.push_back(tile);
            error += deltaerr;
            if(error >= 0.5)
            {
                x += diffX;
                error = error - 1.0;
            }
        }
    }

    // We add the last tile
    Tile* tile = getTile(x2, y2);
    if(tile != nullptr)
        path.push_back(tile);

    return path;
}

std::vector<Tile*> TileContainer::visibleTiles(Tile *startTile, const std::vector<Tile*>& tilesWithinSightRadius)
{
    std::vector<Tile*> tempVector;

    if (!startTile->permitsVision())
        return tempVector;

    // To avoid too many iterations, we split tilesWithinSightRadius on vectors depending on X.
    uint32_t nbTiles = tilesWithinSightRadius.size();
    std::map<int, std::vector<Tile*>> tilesWithinSightRadiusWork;
    for(Tile* tile : tilesWithinSightRadius)
    {
        tilesWithinSightRadiusWork[tile->getX()].push_back(tile);
    }

    // We process all the tiles within sight radius. We start from the start tile and go to the processed tile.
    // While we have sight on the processed tiles, we add them to the returned vector. Once we hit a tile
    // where we don't have vision, we remove the remaining tiles.
    while(nbTiles > 0)
    {
        std::pair<const int, std::vector<Tile*>>& p = *tilesWithinSightRadiusWork.begin();
        // We assume tilesWithinSightRadius is filled with tiles from the closest to farther. It would avoid some calls
        // to tilesBetween if we process from the farther to the closest tiles so we reverse the order
        Tile* tileDest = *p.second.rbegin();

        std::list<Tile*> tiles = tilesBetween(startTile->getX(), startTile->getY(),
            tileDest->getX(), tileDest->getY());

        Tile* lastTile = nullptr;
        bool hasVision = true;
        for(Tile* tile : tiles)
        {
            // We need to differentiate the first tile that stops vision to add it to the list
            // so that we can see the tile stopping vision
            bool hasVisionLast = hasVision;

            // If we change tile diagonally, we check that at least one of the 2 surrounding tiles permits vision
            if(hasVision &&
               (lastTile != nullptr) &&
               (lastTile->getX() != tile->getX()) &&
               (lastTile->getY() != tile->getY()))
            {
                int x = std::min(lastTile->getX(), tile->getX());
                int y = std::min(lastTile->getY(), tile->getY());
                Tile* t1;
                Tile* t2;
                if((lastTile->getX() == x &&
                    lastTile->getY() == y) ||
                   (tile->getX() == x &&
                    tile->getY() == y))
                {
                    t1 = getTile(x + 1, y);
                    t2 = getTile(x, y + 1);
                }
                else
                {
                    t1 = getTile(x, y);
                    t2 = getTile(x + 1, y + 1);
                }

                // If we don't have vision on the 2 tiles, we cannot see through
                if(!t1->permitsVision() && !t2->permitsVision())
                {
                    hasVision = false;
                    // Moreover, we don't want this tile to be added if it was to be
                    // so we set hasVisionLast to false
                    hasVisionLast = false;
                }
            }
            lastTile = tile;

            if(hasVision && !tile->permitsVision())
                hasVision = false;

            if(tilesWithinSightRadiusWork.count(tile->getX()) <= 0)
                continue;

            std::vector<Tile*>& listTiles = tilesWithinSightRadiusWork.at(tile->getX());
            std::vector<Tile*>::iterator it = std::find(listTiles.begin(), listTiles.end(), tile);
            if(it == listTiles.end())
                continue;

            --nbTiles;
            listTiles.erase(it);
            // When a list is empty, we can remove it from the map
            if(listTiles.empty())
                tilesWithinSightRadiusWork.erase(tileDest->getX());

            // If vision has changed on this tile, we add it to the list (If a wall stops vision,
            // we cannot see behind but we can see the wall)
            if(!hasVision && !hasVisionLast)
                continue;

            tempVector.push_back(tile);
        }
    }

    return tempVector;
}
