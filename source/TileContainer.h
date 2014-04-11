#ifndef TILECONTAINER_H
#define TILECONTAINER_H

#include "Tile.h"
#include <sstream>


class TileContainer
{

friend class TileContainersModificator;

public:
   TileContainer();
  ~TileContainer();

  // Game state methods
  void clearTiles();

    //! \brief Adds the given tile on map. The tile coordinates members must be ready.
    //! \returns true if added.
    bool addTile(const Tile& t);

  void setTileNeighbors(Tile *t);

  Tile* getTile(int x, int y) const;
  Tile::TileType getSafeTileType(Tile* tt );
  bool getSafeTileFullness(Tile* tt );

  Tile* firstTile();
  Tile* lastTile();
  Tile::TileType*  getNeighborsTypes( Tile* , Tile::TileType*);
  bool*   getNeighborsFullness( Tile* , bool*);
  unsigned int numTiles();

    //! \brief Returns all the valid tiles in the rectangular region specified by the two corner points given.
    std::vector<Tile*> rectangularRegion(int x1, int y1, int x2, int y2);

    //! \brief Returns all the valid tiles in the curcular region
    //! surrounding the given point and extending outward to the specified radius.
    std::vector<Tile*> circularRegion(int x, int y, double radius) const;

    //! \brief Returns a vector of all the valid tiles which are a neighbor
    //! to one or more tiles in the specified region,
    //! i.e. the "perimeter" of the region extended out one tile.
    std::vector<Tile*> tilesBorderedByRegion(const std::vector<Tile*> &region);

    //! \brief Returns the (up to) 4 nearest neighbor tiles of the tile located at (x, y).
    std::vector<Tile*> neighborTiles(int x, int y);

    //! \brief Gets the map size
    int getMapSizeX() const
    { return mapSizeX; }

    int getMapSizeY() const
    { return mapSizeY; }

protected:
    //! \brief Tile change semaphore
    mutable sem_t tilesLockSemaphore;

    //! \brief The map size
    int mapSizeX;
    int mapSizeY;

    int rr;

    //! \brief Set the map size and memory
    bool allocateMapMemory(int xSize, int ySize);
private:
  Tile **tiles;
};

#endif //TILECONTAINER_H
