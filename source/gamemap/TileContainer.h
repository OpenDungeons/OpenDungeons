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

#ifndef TILECONTAINER_H
#define TILECONTAINER_H

#include <array>
#include <bitset>
#include <list>
#include <sstream>
#include <vector>

class ODPacket;
class TileDistance;
class Tile;

enum class TileType;

class TileContainer
{
public:
    TileContainer(int initTileDistance);
    ~TileContainer();

    //! \brief Clears the mesh and deletes the data structure for all the tiles in the TileContainer.
    void clearTiles();

    //! \brief Adds the given tile on map. The tile coordinates members must be ready.
    //! \returns true if added.
    bool addTile(Tile* t);

    //! \brief Adds the address of a new tile to be stored in this TileContainer.
    void setTileNeighbors(Tile *t);

    /*! \brief Returns a pointer to the tile at location (x, y) (const version).
     *
     * The tile pointers are stored internally in a map so calls to this function
     * have a complexity O(log(N)) where N is the number of tiles in the map.
     * This function does not lock.
     */
    Tile* getTile(int x, int y) const;

    //! \brief This functions exports the needed to retrieve a tile for networking.
    //! The tile informations are not embedded, only the needed to identify the tile
    void tileToPacket(ODPacket& packet, Tile* tile) const;
    Tile* tileFromPacket(ODPacket& packet) const;

    TileType getSafeTileType(const Tile* tt) const;
    bool getSafeTileFullness(const Tile* tt) const;

    //! \brief Gets the tile type of all neighbors
    //! \param tile The tile to be checked.
    //! \returns a TileType[8] array pointer where the info is stored.
    std::array<TileType, 8> getNeighborsTypes(const Tile* tile) const;

    //! \brief Gets the tile fullness of all neighbors
    //! \param tile The tile to be checked.
    //! \returns a bool[8] array pointer where the info is stored.
    std::bitset<8> getNeighborsFullness(const Tile* tile) const;

    //! \brief Returns the number of tile pointers currently stored in this TileContainer.
    unsigned int numTiles();

    //! \brief Returns all the valid tiles in the rectangular region specified by the two corner points given.
    std::vector<Tile*> rectangularRegion(int x1, int y1, int x2, int y2);

    //! \brief Returns all the valid tiles in the curcular region
    //! surrounding the given point and extending outward to the specified radius.
    std::vector<Tile*> circularRegion(int x, int y, int radius);

    //! \brief Returns a vector of all the valid tiles which are a neighbor
    //! to one or more tiles in the specified region,
    //! i.e. the "perimeter" of the region extended out one tile.
    std::vector<Tile*> tilesBorderedByRegion(const std::vector<Tile*> &region);

    //! \brief Returns the (up to) 4 nearest neighbor tiles of the tile located at (x, y).
    const std::vector<Tile*>& neighborTiles(int x, int y) const;

    //! \brief Gets the map size
    int getMapSizeX() const
    { return mMapSizeX; }

    int getMapSizeY() const
    { return mMapSizeY; }

    static bool sortByDistSquared(const TileDistance& tileDist1, const TileDistance& tileDist2);

    /*! \brief Returns a list of valid tiles along a straight line from (x1, y1) to (x2, y2)
     * independently from their fullness or type.
     *
     * This algorithm is from
     * http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
     * A more detailed description of how it works can be found there.
     */
    std::list<Tile*> tilesBetween(int x1, int y1, int x2, int y2);

    //! \brief Returns the tiles visible from the given start tile within tilesWithinSightRadius.
    std::vector<Tile*> visibleTiles(int x, int y, int radius);

protected:
    //! \brief The map size
    int mMapSizeX;
    int mMapSizeY;

    int mRr;

    //! \brief Set the map size and memory
    bool allocateMapMemory(int xSize, int ySize);
private:
    Tile*** mTiles;

    //! \brief Fills mTileDistance that will help to compute a vector with sorted Tiles more efficiently
    void buildTileDistance(int distance);

    //! \brief Helper to compute tile distances more efficiently
    std::vector<TileDistance> mTileDistance;

    //! \brief Stores the highest distance computed. If a bigger distance is asked, mTileDistance will have to be updated by
    //! calling buildTileDistance with the higher distance
    int mTileDistanceComputed;
};

#endif //TILECONTAINER_H
