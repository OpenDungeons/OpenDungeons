#include "Globals.h"
#include "Functions.h"
#include "Tile.h"
#include "GameMap.h"
#include "TrapCannon.h"
//~ #include "MissileObject.h"

TrapCannon::TrapCannon() : ProximityTrap()
{
    reloadTime = 5;
    reloadTimeCounter = reloadTime;
    range = 12;
    minDamage = 4;
    maxDamage = 20;
    cannonHeight = 1.5;
}

std::vector<AttackableObject*> TrapCannon::aimEnemy() 
{
    std::vector<Tile*> visibleTiles = gameMap.visibleTiles(coveredTiles[0], range);
    std::vector<AttackableObject*> enemyObjects = gameMap.getVisibleForce(visibleTiles, getColor(), true);
    if(enemyObjects.size() <= 0) {
        return std::vector<AttackableObject*>();
    }
    // Select an enemy to shoot at.
    AttackableObject* targetEnemy = enemyObjects[randomUint(0, enemyObjects.size()-1)];
    
    std::vector<AttackableObject*> enemies = std::vector<AttackableObject*>();
    enemies.push_back(targetEnemy);
    return enemies;
}