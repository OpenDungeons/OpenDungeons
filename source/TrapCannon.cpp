#include "Tile.h"
#include "GameMap.h"
#include "MissileObject.h"
#include "Random.h"

#include "TrapCannon.h"

TrapCannon::TrapCannon() :
        cannonHeight(1.5)
{
    reloadTime = 5;
    reloadTimeCounter = reloadTime;
    range = 12;
    minDamage = 104;
    maxDamage = 120;
}

std::vector<GameEntity*> TrapCannon::aimEnemy()
{
    std::vector<Tile*> visibleTiles = getGameMap()->visibleTiles(coveredTiles[0], range);
    std::vector<GameEntity*> enemyObjects = getGameMap()->getVisibleForce(visibleTiles, getColor(), true);
    if(enemyObjects.size() <= 0) {
        return std::vector<GameEntity*>();
    }
    // Select an enemy to shoot at.
    GameEntity* targetEnemy = enemyObjects[Random::Uint(0, enemyObjects.size()-1)];
    
    std::vector<GameEntity*> enemies = std::vector<GameEntity*>();
    enemies.push_back(targetEnemy);
    return enemies;
}

void TrapCannon::damage(std::vector<GameEntity*> enemyAttacked)
{
	ProximityTrap::damage(enemyAttacked);
	
	if(enemyAttacked.empty())
		return;
	
	std::cout << "\nAdding cannonball from " << coveredTiles[0]->x << "," << coveredTiles[0]->y << " to " << enemyAttacked[0]->getCoveredTiles()[0]->x << "," << enemyAttacked[0]->getCoveredTiles()[0]->y << std::endl;
	// Create the cannonball to move toward the enemy creature.
	MissileObject *tempMissileObject = new MissileObject(
        "Cannonball", Ogre::Vector3(coveredTiles[0]->x, coveredTiles[0]->y, cannonHeight), *getGameMap());
	tempMissileObject->setMoveSpeed(8.0);
	tempMissileObject->createMesh();
	//TODO: Make this a pseudo newtonian mechanics solver which computes a parabola passing through the cannon
	// and the enemy it is shooting at, add this as 10 or so destinations in the queue instead of just one.
	tempMissileObject->addDestination(enemyAttacked[0]->getCoveredTiles()[0]->x, enemyAttacked[0]->getCoveredTiles()[0]->y, cannonHeight);
	getGameMap()->addMissileObject(tempMissileObject);
}
