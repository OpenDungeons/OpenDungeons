#include "Globals.h"
//~ #include "Functions.h"
#include "Tile.h"
#include "GameMap.h"
#include "TrapBoulder.h"
//~ #include "MissileObject.h"

TrapBoulder::TrapBoulder(int x, int y) : DirectionalTrap(x, y)
{
    reloadTime = -1;
    reloadTimeCounter = reloadTime;
    minDamage = 30;
    maxDamage = 40;
}

std::vector<AttackableObject*> TrapBoulder::aimEnemy() 
{
    std::list<Tile*> tmp = gameMap.lineOfSight(coveredTiles[0]->x, coveredTiles[0]->y, dir.first, dir.second);
    std::vector<Tile*> visibleTiles;
    for(std::list<Tile*>::const_iterator it = tmp.begin(); it != tmp.end(); it++) {
        if((*it)->getTilePassability() == Tile::impassableTile) {
            break;
        }
        visibleTiles.push_back(*it);
    }
    //By defaut, you damage every attackable object in the line.
    std::vector<AttackableObject*> v1 = gameMap.getVisibleForce(visibleTiles, getColor(), true);
    std::vector<AttackableObject*> v2 = gameMap.getVisibleForce(visibleTiles, getColor(), false);
    for(std::vector<AttackableObject*>::const_iterator it = v2.begin(); it != v2.end(); it++) {
        v1.push_back(*it);
    }
    return v1;
}