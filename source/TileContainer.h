#ifndef TILECONTAINER_H
#define TILECONTAINER_H

#include "Tile.h"
#include <sstream>


class TileContainer{

friend class TileContainersModificator;

public:
   TileContainer();
  ~TileContainer();
  // Game state methods
  int  insert(int ii , int jj , Tile tt);
  void clearTiles();
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
  int getMapSizeX() const;
  int getMapSizeY() const;
  int setMapSizeX(int );
  int setMapSizeY(int );

protected:
  mutable sem_t tilesLockSemaphore;
  int mapSizeX;
  int mapSizeY;
  int rr;
  
private:
  Tile **tiles;
  bool symetry;




  Tile* auxTilesArray;
    
    };

#endif //TILECONTAINER_H
