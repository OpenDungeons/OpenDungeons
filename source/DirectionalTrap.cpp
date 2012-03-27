#include "DirectionalTrap.h"
#include "Tile.h"
#include "GameMap.h"

DirectionalTrap::DirectionalTrap(int xdir, int ydir)
{
}

std::pair<int, int> DirectionalTrap::projection_on_border(int xdir, int ydir) {
    int a = (ydir - coveredTiles[0]->y)/(xdir - coveredTiles[0]->x);
    int b = ydir - a*xdir;
    int tmp;
    if(b >= 0 && b <= getGameMap()->maxY()) {
        return std::pair<int, int>(0, b);
    }
    else if(-(b/a) >= 0 && -(b/a) <= getGameMap()->maxX()) {
        return std::pair<int, int>((-b)/a, 0);
    }
    tmp = a*getGameMap()->maxX() + b;
    if(tmp >= 0 && tmp <= getGameMap()->maxY()) {
        return std::pair<int, int>(getGameMap()->maxX(), tmp);
    }
    return std::pair<int, int>((getGameMap()->maxY() - b)/a, getGameMap()->maxY());
}
