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

#include "entities/Tile.h"

#include "network/ODPacket.h"
#include "utils/Helper.h"
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

    void computeTileDistances(double coefNorth, double coefSouth, const TileDistance& tileDistance,
        uint32_t indexTileDistance)
    {
        // A tile can only hide tiles behind (x > tile.x and y > tile.y)
        if(tileDistance.getDiffX() < getDiffX())
            return;
        if(tileDistance.getDiffY() < getDiffY())
            return;

        // We don't want a tile to hide itself
        if((tileDistance.getDiffX() == getDiffX()) &&
           (tileDistance.getDiffY() == getDiffY()))
        {
            return;
        }

        if(getType() == TileDistance::TileDistanceType::Horizontal)
        {
            // For horizontal tiles, we hide following tiles (x > tile.x). But we process
            // north tiles normally
            if(tileDistance.getType() == TileDistance::TileDistanceType::Horizontal)
            {
                addHiddenTileSouth(indexTileDistance, 1.0);
                return;
            }

            double xTileDeb = static_cast<double>(tileDistance.getDiffX()) - 0.5;
            double xTileEnd = xTileDeb + 1.0;
            double yTileDeb = static_cast<double>(tileDistance.getDiffY()) - 0.5;
            double yTileEnd = yTileDeb + 1.0;
            double yHideDebNorth = coefNorth * xTileDeb;
            double yHideEndNorth = coefNorth * xTileEnd;

            // If the tile is over the North ray, it is not hidden
            if(yHideEndNorth <= yTileDeb)
                return;

            // We check which part of the tile is hidden
            if((yHideDebNorth >= yTileDeb) &&
               (yHideEndNorth <= yTileEnd))
            {
                // The ray hits the left side of the tile and the right side.
                // The south part is partially hidden
                double hiddenArea = (yHideEndNorth - yHideDebNorth) / 2.0;
                hiddenArea += yHideDebNorth - yTileDeb;
                addHiddenTileSouth(indexTileDistance, hiddenArea);
            }
            else if((yHideDebNorth < yTileDeb) &&
                    (yHideEndNorth > yTileDeb))
            {
                // The ray hits the bottom side of the tile but hits the right side. We compute
                // the south visible part
                double xHit = yTileDeb / coefNorth;
                double hiddenArea = (yHideEndNorth - yTileDeb) * (xTileEnd - xHit) / 2.0;
                addHiddenTileSouth(indexTileDistance, hiddenArea);
            }
            else if((yHideDebNorth < yTileEnd) &&
                    (yHideEndNorth > yTileEnd))
            {
                // The ray hits the left side of the tile but is over the right side. We compute
                // the hidden part on north.
                double xHit = yTileEnd / coefNorth;
                double visibleArea = (yTileEnd - yHideDebNorth) * (xHit - xTileDeb) / 2.0;
                addHiddenTileSouth(indexTileDistance, 1.0 - visibleArea);
            }
            else
            {
                // The entire tile is hidden
                addHiddenTileSouth(indexTileDistance, 1.0);
            }

            return;
        }

        double xTileDeb = static_cast<double>(tileDistance.getDiffX()) - 0.5;
        double xTileEnd = xTileDeb + 1.0;
        double yTileDeb = static_cast<double>(tileDistance.getDiffY()) - 0.5;
        double yTileEnd = yTileDeb + 1.0;

        // We check if the current tile is hidden by the tile. To consider that the
        // tile is hidden by the south, as we know the angle will be between 0 and 45 degrees,
        // we consider that the tile has to be hit by the ray passing through the hiding tile
        // on the left side of the tile (otherwise, the hidden part will be too small).
        double yHideDebSouth = coefSouth * xTileDeb;
        double yHideEndSouth = coefSouth * xTileEnd;
        double yHideDebNorth = coefNorth * xTileDeb;
        double yHideEndNorth = coefNorth * xTileEnd;
        // We check if at least a part of the tile is hidden
        if((yHideDebSouth < yTileEnd) &&
           (yHideEndNorth > yTileDeb))
        {
            // At least a part of this tile is hidden
            if((yHideDebSouth >= yTileDeb) &&
               (yHideEndSouth <= yTileEnd))
            {
                // The ray hits the left side of the tile and the right side.
                // The south part is partially hidden
                // The visible part is composed from a square between the tile inferior part and
                // the triangle made by the ray
                double visibleArea = (yHideEndSouth - yHideDebSouth) / 2.0;
                visibleArea += yHideDebSouth - yTileDeb;
                addHiddenTileNorth(indexTileDistance, 1.0 - visibleArea);
            }
            else if((yHideDebSouth < yTileDeb) &&
                    (yHideEndSouth > yTileDeb))
            {
                // The ray hits the bottom side of the tile but hits the right side. We compute
                // the south visible part
                double xHit = yTileDeb / coefSouth;
                double visibleArea = (yHideEndSouth - yTileDeb) * (xTileEnd - xHit) / 2.0;
                addHiddenTileNorth(indexTileDistance, 1.0 - visibleArea);
            }
            else if((yHideDebSouth < yTileEnd) &&
                    (yHideEndSouth > yTileEnd))
            {
                // The ray hits the left side of the tile but is over the right side. We compute
                // the hidden part on north.
                double xHit = yTileEnd / coefSouth;
                double hiddenArea = (yTileEnd - yHideDebSouth) * (xHit - xTileDeb) / 2.0;
                addHiddenTileNorth(indexTileDistance, hiddenArea);

            }
            else if((yHideDebNorth >= yTileDeb) &&
               (yHideEndNorth <= yTileEnd))
            {
                double hiddenArea = (yHideEndNorth - yHideDebNorth) / 2.0;
                hiddenArea += yHideDebNorth - yTileDeb;
                addHiddenTileSouth(indexTileDistance, hiddenArea);
            }
            else if((yHideDebNorth < yTileDeb) &&
                    (yHideEndNorth > yTileDeb))
            {
                // The ray hits the bottom side of the tile but hits the right side. We compute
                // the south visible part
                double xHit = yTileDeb / coefNorth;
                double hiddenArea = (yHideEndNorth - yTileDeb) * (xTileEnd - xHit) / 2.0;
                addHiddenTileSouth(indexTileDistance, hiddenArea);
            }
            else if((yHideDebNorth < yTileEnd) &&
                    (yHideEndNorth > yTileEnd))
            {
                // The ray hits the left side of the tile but is over the right side. We compute
                // the hidden part on north.
                double xHit = yTileEnd / coefNorth;
                double visibleArea = (yTileEnd - yHideDebNorth) * (xHit - xTileDeb) / 2.0;
                addHiddenTileSouth(indexTileDistance, 1.0 - visibleArea);
            }
            else
            {
                // The entire tile is hidden
                addHiddenTileSouth(indexTileDistance, 1.0);
            }
        }
    }

    void print(const std::vector<TileDistance>& tileDistance)
    {
        std::string log = "TileDistance x=" + Helper::toString(getDiffX())
            + ", y=" + Helper::toString(getDiffY())
            + ", squared=" + Helper::toString(getDistSquared());
        for(const std::pair<uint32_t, double>& p : mHiddenTilesNorth)
        {
            const TileDistance& tile = tileDistance[p.first];
            log += ", TileDistanceNorth x=" + Helper::toString(tile.getDiffX())
                + ", y=" + Helper::toString(tile.getDiffY())
                + ", val=" + Helper::toString(p.second);
        }
        for(const std::pair<uint32_t, double>& p : mHiddenTilesSouth)
        {
            const TileDistance& tile = tileDistance[p.first];
            log += ", TileDistanceSouth x=" + Helper::toString(tile.getDiffX())
                + ", y=" + Helper::toString(tile.getDiffY())
                + ", val=" + Helper::toString(p.second);
        }

        LogManager::getSingleton().logMessage(log);
    }

    const std::vector<std::pair<uint32_t, double>>& getHiddenTilesNorth() const
    {
        return mHiddenTilesNorth;
    }

    const std::vector<std::pair<uint32_t, double>>& getHiddenTilesSouth() const
    {
        return mHiddenTilesSouth;
    }

private:
    void addHiddenTileNorth(uint32_t indexTile, double hiddenPercent)
    {
        mHiddenTilesNorth.push_back(std::pair<uint32_t, double>(indexTile, hiddenPercent));
    }

    void addHiddenTileSouth(uint32_t indexTile, double hiddenPercent)
    {
        mHiddenTilesSouth.push_back(std::pair<uint32_t, double>(indexTile, hiddenPercent));
    }

    int mDiffX;
    int mDiffY;
    TileDistanceType mType;
    int mDistSquared;
    std::vector<std::pair<uint32_t, double>> mHiddenTilesNorth;
    std::vector<std::pair<uint32_t, double>> mHiddenTilesSouth;
};

class TileDistanceProcess
{
public:
    TileDistanceProcess(const TileDistance& tileDistance, Tile* tile):
        mTileDistance(tileDistance),
        mTile(tile),
        mHiddenValueNorth(0.0),
        mHiddenValueSouth(0.0)
    {
    }

    inline const TileDistance& getTileDistance() const
    {
        return mTileDistance;
    }

    void addHiddenValueNorth(double val)
    {
        // We only add the highest value
        if(val <= mHiddenValueNorth)
            return;

        mHiddenValueNorth = val;
    }

    void addHiddenValueSouth(double val)
    {
        // We only add the highest value
        if(val <= mHiddenValueSouth)
            return;

        mHiddenValueSouth = val;
    }

    inline bool isTileVisible() const
    {
        return (mHiddenValueNorth + mHiddenValueSouth) <= 0.5;
    }

    inline double getHiddenValueNorth() const
    {
        return mHiddenValueNorth;
    }

    inline double getHiddenValueSouth() const
    {
        return mHiddenValueSouth;
    }

    inline Tile* getTile() const
    {
        return mTile;
    }

private:
    const TileDistance& mTileDistance;
    Tile* mTile;
    double mHiddenValueNorth;
    double mHiddenValueSouth;
};

TileContainer::TileContainer(int initTileDistance):
    mMapSizeX(0),
    mMapSizeY(0),
    mRr(0),
    mTiles(nullptr),
    mTileDistanceComputed(0)
{
    buildTileDistance(initTileDistance);
}

TileSet::TileSet(const Ogre::Vector3& scale) :
    mTileValues(static_cast<uint32_t>(TileVisual::countTileVisual), std::vector<TileSetValue>(16)),
    mScale(scale),
    mTileLinks(std::vector<uint32_t>(static_cast<uint32_t>(TileVisual::countTileVisual), 0))
{
}

std::vector<TileSetValue>& TileSet::configureTileValues(TileVisual tileVisual)
{
    uint32_t tileTypeNumber = static_cast<uint32_t>(tileVisual);
    if(tileTypeNumber >= mTileValues.size())
    {
        OD_ASSERT_TRUE_MSG(false, "Trying to get unknow tileType=" + Tile::tileVisualToString(tileVisual));
        return mTileValues.at(0);
    }

    return mTileValues[tileTypeNumber];
}

const std::vector<TileSetValue>& TileSet::getTileValues(TileVisual tileVisual) const
{
    uint32_t tileTypeNumber = static_cast<uint32_t>(tileVisual);
    if(tileTypeNumber >= mTileValues.size())
    {
        OD_ASSERT_TRUE_MSG(false, "Trying to get unknow tileType=" + Tile::tileVisualToString(tileVisual));
        return mTileValues.at(0);
    }

    return mTileValues[tileTypeNumber];
}

void TileSet::addTileLink(TileVisual tileVisual1, TileVisual tileVisual2)
{
    uint32_t intTile1Visual = static_cast<uint32_t>(tileVisual1);
    uint32_t intTile2Visual = static_cast<uint32_t>(tileVisual2);
    if(intTile1Visual >= mTileLinks.size())
    {
        OD_ASSERT_TRUE_MSG(false, "TileVisual=" + Tile::tileVisualToString(tileVisual1));
        return;
    }
    if(intTile2Visual >= mTileLinks.size())
    {
        OD_ASSERT_TRUE_MSG(false, "TileVisual=" + Tile::tileVisualToString(tileVisual2));
        return;
    }

    uint32_t tileLink;
    tileLink = 1 << intTile2Visual;
    mTileLinks[intTile1Visual] |= tileLink;

    // We want a commutative relationship
    tileLink = 1 << intTile1Visual;
    mTileLinks[intTile2Visual] |= tileLink;
}

bool TileSet::areLinked(const Tile* tile1, const Tile* tile2) const
{
    // Check if the tile visual is linkable
    uint32_t intTile1Visual = static_cast<uint32_t>(tile1->getTileVisual());
    uint32_t intTile2Visual = static_cast<uint32_t>(tile2->getTileVisual());
    uint32_t linkValue = 1 << intTile2Visual;
    linkValue &= mTileLinks[intTile1Visual];

    if(linkValue == 0)
        return false;

    return true;
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
                mTiles[ii][jj]->destroyMesh();
                delete mTiles[ii][jj];
            }
            delete[] mTiles[ii];
        }
        delete[] mTiles;
        mTiles = nullptr;
    }
    mMapSizeX = 0;
    mMapSizeY = 0;
}

bool TileContainer::addTile(Tile* t)
{
    int x = t->getX();
    int y = t->getY();

    if (x < getMapSizeX() && y < getMapSizeY() && x >= 0 && y >= 0)
    {
        if(mTiles[x][y] != nullptr)
        {
            mTiles[x][y]->destroyMesh();
            delete mTiles[x][y];
        }
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
    Tile* tile = getTile(x, y);
    if(tile == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "tile=" + Helper::toString(x) + "," + Helper::toString(y));
    }
    return tile;
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

TileType TileContainer::getSafeTileType(const Tile* tt) const
{
    return (tt == nullptr) ? TileType::nullTileType : tt->getType();
}

bool TileContainer::getSafeTileFullness(const Tile* tt) const
{
    return (tt == nullptr) ? false : (tt->getFullness() > 0);
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

    std::vector<std::vector<bool>> tilesToRefresh(getMapSizeX(), std::vector<bool>(getMapSizeY(), false));
    for (Tile* t1 : region)
    {
        if(!tilesToRefresh[t1->getX()][t1->getY()])
        {
            tilesToRefresh[t1->getX()][t1->getY()] = true;
            returnList.push_back(t1);
        }

        // Get the tiles bordering the current tile and loop over them.
        for (Tile* t2 : t1->getAllNeighbors())
        {
            if(tilesToRefresh[t2->getX()][t2->getY()])
                continue;

            tilesToRefresh[t2->getX()][t2->getY()] = true;
            returnList.push_back(t2);
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
    mTileDistance.clear();
    for(int y = 0; y <= distance; ++y)
    {
        for(int x = y; x <= distance; ++x)
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

    // We have filled the tile distance vector. Now, we fill how each tile hides the
    // other ones when they mask vision to help calculate visible tiles
    for(TileDistance& tileDistance : mTileDistance)
    {
        // We don't process the first tile
        if(tileDistance.getDiffX() == 0 && tileDistance.getDiffY() == 0)
            continue;

        // Other tiles can hide with their down side and their up side other tiles
        // or diagonal tiles (but not Horizontal tiles)
        // We compute the tiles hidden from the south. In this case, only tiles with
        // x > tile.x can be hidden
        double coefNorth = (static_cast<double>(tileDistance.getDiffY()) + 0.5) / (static_cast<double>(tileDistance.getDiffX()) - 0.5);
        double coefSouth = (static_cast<double>(tileDistance.getDiffY()) - 0.5) / (static_cast<double>(tileDistance.getDiffX()) + 0.5);
        for(uint32_t index = 0; index < mTileDistance.size(); ++index)
        {
            const TileDistance& tileDistance2 = mTileDistance[index];
            tileDistance.computeTileDistances(coefNorth, coefSouth, tileDistance2, index);
        }
    }

    mTileDistanceComputed = distance;
}

bool TileContainer::sortByDistSquared(const TileDistance& tileDist1, const TileDistance& tileDist2)
{
    return tileDist1.getDistSquared() < tileDist2.getDistSquared();
}

std::list<Tile*> TileContainer::tilesBetween(int x1, int y1, int x2, int y2) const
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

std::list<Tile*> TileContainer::tilesBetweenWithoutDiagonals(int x1, int y1, int x2, int y2) const
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
                tile = getTile(x + diffX, y);
                if(tile == nullptr)
                    break;

                path.push_back(tile);
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
                tile = getTile(x, y + diffY);
                if(tile == nullptr)
                    break;

                path.push_back(tile);
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

std::vector<Tile*> TileContainer::visibleTiles(int x, int y, int radius)
{
    // To compute the tiles within this region, we use the symmetry of the square. That's why we mix tile x/y coordinate
    // with tileDist diffX/diffY. More explanation can be found in the buildTileDistance function
    std::vector<Tile*> returnList;

    if(radius > mTileDistanceComputed)
        buildTileDistance(radius);

    int radiusSquared = radius * radius;

    // To have all the tiles around, we process mTileDistance 8 times.
    // Because of the symmetry, there will be some duplicates (horizontal and diagonal
    // tiles). We will process in, this order (c being the starting tile):
    // 514
    // 2c0
    // 637
    // Then, we will have to merge diagonal/horizontal tiles
    // Because we want the index to be correct, we will add tiles even when null in tilesProcess
    std::vector<TileDistanceProcess> tilesProcess[8];
    for(uint32_t k = 0; k < 8; ++k)
    {
        for(const TileDistance& tileDist : mTileDistance)
        {
            if(tileDist.getDistSquared() > radiusSquared)
                break;

            switch(k)
            {
                case 0:
                {
                    Tile* tile = getTile(x + tileDist.getDiffX(), y + tileDist.getDiffY());
                    tilesProcess[k].push_back(TileDistanceProcess(tileDist, tile));
                    break;
                }
                case 1:
                {
                    Tile* tile = getTile(x + tileDist.getDiffY(), y - tileDist.getDiffX());
                    tilesProcess[k].push_back(TileDistanceProcess(tileDist, tile));
                    break;
                }
                case 2:
                {
                    Tile* tile = getTile(x - tileDist.getDiffX(), y - tileDist.getDiffY());
                    tilesProcess[k].push_back(TileDistanceProcess(tileDist, tile));
                    break;
                }
                case 3:
                {
                    Tile* tile = getTile(x - tileDist.getDiffY(), y + tileDist.getDiffX());
                    tilesProcess[k].push_back(TileDistanceProcess(tileDist, tile));
                    break;
                }
                case 4:
                {
                    Tile* tile = getTile(x + tileDist.getDiffY(), y + tileDist.getDiffX());
                    tilesProcess[k].push_back(TileDistanceProcess(tileDist, tile));
                    break;
                }
                case 5:
                {
                    Tile* tile = getTile(x + tileDist.getDiffX(), y - tileDist.getDiffY());
                    tilesProcess[k].push_back(TileDistanceProcess(tileDist, tile));
                    break;
                }
                case 6:
                {
                    Tile* tile = getTile(x - tileDist.getDiffY(), y - tileDist.getDiffX());
                    tilesProcess[k].push_back(TileDistanceProcess(tileDist, tile));
                    break;
                }
                case 7:
                {
                    Tile* tile = getTile(x - tileDist.getDiffX(), y + tileDist.getDiffY());
                    tilesProcess[k].push_back(TileDistanceProcess(tileDist, tile));
                    break;
                }
                default:
                    break;
            }
        }
    }

    // The array of tiles is filled. Now, we apply the visibility.
    for(uint32_t k = 0; k < 8; ++k)
    {
        for(TileDistanceProcess& tileDistanceProcess : tilesProcess[k])
        {
            if(tileDistanceProcess.getTile() == nullptr)
                continue;

            if(tileDistanceProcess.getTile()->permitsVision())
                continue;

            // The tile hides vision. We process tiles it hides
            for(const std::pair<uint32_t, double>& p : tileDistanceProcess.getTileDistance().getHiddenTilesNorth())
            {
                // mTileDistance might be bigger than the actual vector because it can include tiles
                // farther than the ones currently computed (for example if sight < computedSight)
                if(p.first >= tilesProcess[k].size())
                    continue;

                tilesProcess[k][p.first].addHiddenValueNorth(p.second);
            }
            for(const std::pair<uint32_t, double>& p : tileDistanceProcess.getTileDistance().getHiddenTilesSouth())
            {
                // mTileDistance might be bigger than the actual vector because it can include tiles
                // farther than the ones currently computed (for example if sight < computedSight)
                if(p.first >= tilesProcess[k].size())
                    continue;

                tilesProcess[k][p.first].addHiddenValueSouth(p.second);
            }
        }
    }

    // Now, we process all the tiles. Note that horizontal tiles are common for 2 consecutive
    // vectors in tilesProcess and that diagonal tiles should be merged.
    // The 8 vectors have the same size
    for(uint32_t i = 0; i < tilesProcess[0].size(); ++i)
    {
        for(uint32_t k = 0; k < 8; ++k)
        {
            TileDistanceProcess& tileDistanceProcess = tilesProcess[k][i];
            if(tileDistanceProcess.getTile() == nullptr)
                continue;

            // Because horizontal tiles are common, we don't process them for the 4 last vectors
            if((tileDistanceProcess.getTileDistance().getType() == TileDistance::TileDistanceType::Horizontal) &&
               (k > 3))
            {
                continue;
            }

            // Diagonal tiles need to be merged (because south hiding and north hiding are not
            // computed within the same array). They will be processed for k < 4
            if((tileDistanceProcess.getTileDistance().getType() == TileDistance::TileDistanceType::Diagonal) &&
               (k > 3))
            {
                continue;
            }

            if(tileDistanceProcess.getTileDistance().getType() == TileDistance::TileDistanceType::Diagonal)
            {
                // We merge diagonal tiles. Because they are inverted, south hidden value becomes north and vice-versa
                TileDistanceProcess& tileDistanceProcess2 = tilesProcess[k + 4][i];
                tileDistanceProcess.addHiddenValueNorth(tileDistanceProcess2.getHiddenValueSouth());
                tileDistanceProcess.addHiddenValueSouth(tileDistanceProcess2.getHiddenValueNorth());
            }

            if(!tileDistanceProcess.isTileVisible())
                continue;

            returnList.push_back(tileDistanceProcess.getTile());
        }
    }
    return returnList;
}
