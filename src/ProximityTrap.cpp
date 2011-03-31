#include "Globals.h"
#include "Functions.h"
#include "ProximityTrap.h"
//~ #include "MissileObject.h"

ProximityTrap::ProximityTrap()
	: Trap()
{
	reloadTime = 10;
	reloadTimeCounter = reloadTime;
	range = 10;
	minDamage = 5;
	maxDamage = 20;
}

std::vector<AttackableObject*> ProximityTrap::aimEnemy() 
{
	std::vector<Tile*> visibleTiles = gameMap.visibleTiles(coveredTiles[0], range);
	std::vector<AttackableObject*> enemyObjects = gameMap.getVisibleForce(visibleTiles, getColor(), true);
	if(enemyObjects.size() <= 0)
        return std::vector<AttackableObject*>();
    // Select an enemy to shoot at.
    AttackableObject* targetEnemy = enemyObjects[randomUint(0, enemyObjects.size()-1)];
    
    std::vector<AttackableObject*> enemies = std::vector<AttackableObject*>();
    enemies.push_back(targetEnemy);
    return enemies;
}

void ProximityTrap::damage(std::vector<AttackableObject*> enemyAttacked) 
{
    for(unsigned i=0;i<enemyAttacked.size();++i) 
    {
		std::cout << "\nCannon firing. Doing damage to enemy: " << enemyAttacked[i]->getName();

		// Shoot the targeted enemy.
		enemyAttacked[i]->takeDamage(randomDouble(minDamage, maxDamage), enemyAttacked[i]->getCoveredTiles()[0]);
    }
}

bool ProximityTrap::doUpkeep()
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

