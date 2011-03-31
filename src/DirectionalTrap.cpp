#include "Globals.h"
#include "Functions.h"
#include "DirectionalTrap.h"
#include "Tile.h"
#include "GameMap.h"
//~ #include "MissileObject.h"

DirectionalTrap::DirectionalTrap(int xdir, int ydir) : Trap()
{
    reloadTime = 10;
    reloadTimeCounter = reloadTime;
    minDamage = 5;
    maxDamage = 20;
    dir = projection_on_border(xdir, ydir);
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

std::vector<AttackableObject*> DirectionalTrap::aimEnemy() 
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

void DirectionalTrap::damage(std::vector<AttackableObject*> enemyAttacked) 
{
    for(unsigned i=0;i<enemyAttacked.size();++i) 
    {
        std::cout << "\nBoulder passing. Doing damage to enemy: " << enemyAttacked[i]->getName();

        // Shoot the targeted enemy.
        enemyAttacked[i]->takeDamage(randomDouble(minDamage, maxDamage), enemyAttacked[i]->getCoveredTiles()[0]);
    }
}

bool DirectionalTrap::doUpkeep()
{
    if(reloadTimeCounter > 0)
    {
        reloadTimeCounter--;
        return true;
    }
    
    std::vector<AttackableObject*> enemyAttacked = aimEnemy();
    
    damage(enemyAttacked);

    if(enemyAttacked.size() > 0)
    {
        // Begin the reload countdown.
        reloadTimeCounter = reloadTime;

        /*
        // Create the cannonball to move toward the enemy creature.
        MissileObject *tempMissileObject = new MissileObject("Cannonball", Ogre::Vector3(coveredTiles[0]->x, coveredTiles[0]->y, cannonHeight));
        tempMissileObject->setMoveSpeed(8.0);
        tempMissileObject->createMesh();
        //TODO: Make this a pseudo newtonian mechanics solver which computes a parabola passing through the cannon and the enemy it is shooting at, add this as 10 or so destinations in the queue instead of just one.
        tempMissileObject->addDestination(targetEnemy->getCoveredTiles()[0]->x, targetEnemy->getCoveredTiles()[0]->y, cannonHeight);
        gameMap.addMissileObject(tempMissileObject);
        */
    }

    return true;
}