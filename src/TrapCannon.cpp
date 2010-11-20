#include "Globals.h"
#include "Functions.h"
#include "TrapCannon.h"
#include "MissileObject.h"

TrapCannon::TrapCannon()
	: Trap()
{
	reloadTime = 5;
	reloadTimeCounter = reloadTime;
	range = 20;
	minDamage = 4;
	maxDamage = 20;
}

void TrapCannon::doUpkeep()
{
	if(reloadTimeCounter > 0)
	{
		std::cout << "\n\n\n\n\n\n\n\nCannon reloading: " << reloadTimeCounter;
		reloadTimeCounter--;
		return;
	}
		
	std::cout << "\n\n\n\n\n\n\n\nCannon upkeep";
	std::vector<Tile*> visibleTiles = gameMap.visibleTiles(coveredTiles[0], range);
	std::vector<AttackableObject*> enemyObjects = gameMap.getVisibleForce(visibleTiles, getColor(), true);
	std::cout << "\nCannon sees " << enemyObjects.size() << " enemies.\n";

	if(enemyObjects.size() > 0)
	{
		// Select an enemy to shoot at.
		AttackableObject *targetEnemy = enemyObjects[randomUint(0, enemyObjects.size()-1)];
		std::cout << "\n\n\nCannon firing. Doing damage to enemy: " << targetEnemy->getName() << "\n\n\n";

		// Shoot the targeted enemy.
		targetEnemy->takeDamage(randomDouble(minDamage, maxDamage), targetEnemy->getCoveredTiles()[0]);

		// Begin the reload countdown.
		reloadTimeCounter = reloadTime;

		// Create the cannonball to move toward the enemy creature.
		MissileObject *tempMissileObject = new MissileObject("Cannonball", Ogre::Vector3(coveredTiles[0]->x, coveredTiles[0]->y, 1.5));
		gameMap.addMissileObject(tempMissileObject);
		tempMissileObject->createMesh();
	}
}

