#ifndef TILECONTAINER_H
#define TILECONTAINER_H

#include "Tile.h"
#include <sstream>


class TileContainer{

friend class TileContainersModificator;

public:
   TileContainer();
   TileContainer( TileContainer const&);
   TileContainer operator=( TileContainer const&);

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

    //! \brief Set the map size and memory
    bool allocateMapMemory(int xSize, int ySize);

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

private:
  Tile **tiles;
  bool symetry;




  Tile* auxTilesArray;

    };

#endif //TILECONTAINER_H
