#ifndef TILECONTAINERSMODIFICATOR_H
#define TILECONTAINERSMODIFICATOR_H

#include <vector>
#include "TileContainer.h"
#include "Tile.h"

class Tile;




class TileContainersModificator{


  public:

    TileContainer* getContainer() {return container; }
    void setContainer( TileContainer* container ) { this->container =  container; }
    std::vector<Tile*> rectangularRegion(int x1, int y1, int x2, int y2);
    std::vector<Tile*> circularRegion(int x, int y, double radius);
    std::vector<Tile*> tilesBorderedByRegion(
	const std::vector<Tile*> &region);
    std::vector<Tile*> neighborTiles(int x, int y);
    std::vector<Tile*> neighborTiles(Tile *t);

    TileContainer* create(int x , int y , Tile::TileType tt );

    TileContainer* copy(TileContainer* tc1 , TileContainer* tc2 );

    TileContainer* embed(TileContainer* tc1 , TileContainer* tc2 );

    TileContainer* rotate90(TileContainer* tc1 );

    TileContainer* reflectX(TileContainer* tc);

    TileContainer* reflectY(TileContainer* tc);

    TileContainer* tilePermute(int x , int y , Tile::TileType tt );

    TileContainer* clone(TileContainer* tc1 );

    TileContainer* tileReplace( TileContainer* tc1 , Tile::TileType tt1, Tile::TileType tt2  );



  private:
    TileContainer* container;


};

#endif //TILECONTAINERSMODIFICATOR_H
