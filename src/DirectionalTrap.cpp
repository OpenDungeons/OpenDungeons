#include "Globals.h"
#include "Functions.h"
#include "DirectionalTrap.h"
#include "Tile.h"
#include "GameMap.h"
//~ #include "MissileObject.h"

DirectionalTrap::DirectionalTrap(int xdir, int ydir)
{
}

std::pair<int, int> DirectionalTrap::projection_on_border(int xdir, int ydir) {
    int a = (ydir - coveredTiles[0]->y)/(xdir - coveredTiles[0]->x);
    int b = ydir - a*xdir;
    int tmp;
    if(b >= 0 && b <= gameMap.maxY()) {
        return std::pair<int, int>(0, b);
    }
    else if(-(b/a) >= 0 && -(b/a) <= gameMap.maxX()) {
        return std::pair<int, int>((-b)/a, 0);
    }
    tmp = a*gameMap.maxX() + b;
    if(tmp >= 0 && tmp <= gameMap.maxY()) {
        return std::pair<int, int>(gameMap.maxX(), tmp);
    }
    return std::pair<int, int>((gameMap.maxY() - b)/a, gameMap.maxY());
}
