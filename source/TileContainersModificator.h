#ifndef TILECONTAINERSMODIFICATOR_H
#define TILECONTAINERSMODIFICATOR_H

#include <vector>
#include "TileContainer.h"
#include "Tile.h"

class Tile;




class TileContainersModificator{


  public:

    TileContainer* getContainer() { return container; };
    void setContainer( TileContainer* container ) { this->container =  container; };
    std::vector<Tile*> rectangularRegion(int x1, int y1, int x2, int y2);
    std::vector<Tile*> circularRegion(int x, int y, double radius);
    std::vector<Tile*> tilesBorderedByRegion(
	const std::vector<Tile*> &region);
    std::vector<Tile*> neighborTiles(int x, int y);
    std::vector<Tile*> neighborTiles(Tile *t);

  private:
    TileContainer* container;


};

#endif //TILECONTAINERSMODIFICATOR_H
