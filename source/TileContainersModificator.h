#ifndef TILECONTAINERSMODIFICATOR_H
#define TILECONTAINERSMODIFICATOR_H

#include <vector>
class Tile;




class TileContainersModificator{


public:
  std::vector<Tile*> rectangularRegion(int x1, int y1, int x2, int y2);
  std::vector<Tile*> circularRegion(int x, int y, double radius);
  std::vector<Tile*> tilesBorderedByRegion(
					   const std::vector<Tile*> &region);





    };

#endif //TILECONTAINERSMODIFICATOR_H
