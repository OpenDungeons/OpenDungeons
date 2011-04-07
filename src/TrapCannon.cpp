#include "Globals.h"
#include "Functions.h"
#include "Tile.h"
#include "GameMap.h"
#include "TrapCannon.h"
#include "MissileObject.h"

TrapCannon::TrapCannon() : ProximityTrap()
{
    reloadTime = 5;
    reloadTimeCounter = reloadTime;
    range = 12;
    minDamage = 104;
    maxDamage = 120;
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

void TrapCannon::damage(std::vector<AttackableObject*> enemyAttacked) 
{
	ProximityTrap::damage(enemyAttacked);
	
	if(enemyAttacked.empty())
		return;
	
	std::cout << "\nAdding cannonball from " << coveredTiles[0]->x << "," << coveredTiles[0]->y << " to " << enemyAttacked[0]->getCoveredTiles()[0]->x << "," << enemyAttacked[0]->getCoveredTiles()[0]->y << std::endl;
	// Create the cannonball to move toward the enemy creature.
	MissileObject *tempMissileObject = new MissileObject("Cannonball", Ogre::Vector3(coveredTiles[0]->x, coveredTiles[0]->y, cannonHeight));
	tempMissileObject->setMoveSpeed(8.0);
	tempMissileObject->createMesh();
	//TODO: Make this a pseudo newtonian mechanics solver which computes a parabola passing through the cannon
	// and the enemy it is shooting at, add this as 10 or so destinations in the queue instead of just one.
	tempMissileObject->addDestination(enemyAttacked[0]->getCoveredTiles()[0]->x, enemyAttacked[0]->getCoveredTiles()[0]->y, cannonHeight);
	gameMap.addMissileObject(tempMissileObject);
}
