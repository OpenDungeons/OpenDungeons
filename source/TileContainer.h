#ifndef TILECONTAINER_H
#define TILECONTAINER_H

#include "Tile.h"
#include <sstream>


class TileContainer{

public:
   TileContainer();
  ~TileContainer();
  // Game state methods
  int  insert(int ii , int jj , Tile tt);
  void clearTiles();
  void map_clear(Tile **tt);
  void addTile(Tile *t);
  
  void setTileNeighbors(Tile *t);

  Tile* getTile(int x, int y) const;
  Tile::TileType getSafeTileType(Tile* tt );
  bool getSafeTileFullness(Tile* tt );

  Tile* firstTile();
  Tile* lastTile();
  Tile::TileType*  getNeighborsTypes( Tile* , Tile::TileType*);
  bool*   getNeighborsFullness( Tile* , bool*);
  unsigned int numTiles();
  int allocateMapMemory(int xSize, int ySize);

private:
  Tile **tiles;
  const static int mapSizeX = 400,  mapSizeY = 400;
  mutable sem_t tilesLockSemaphore;
  Tile* auxTilesArray;
    
    };

#endif //TILECONTAINER_H
