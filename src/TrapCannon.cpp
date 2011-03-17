#include "Globals.h"
#include "Functions.h"
#include "TrapCannon.h"
#include "MissileObject.h"
#include "GameMap.h"
#include "AttackableObject.h"
#include "Tile.h"

TrapCannon::TrapCannon() :
    Trap()
{
    reloadTime = 5;
    reloadTimeCounter = reloadTime;
    range = 12;
    minDamage = 4;
    maxDamage = 20;
    cannonHeight = 1.5;
}

bool TrapCannon::doUpkeep()
{
    if (reloadTimeCounter > 0)
    {
        --reloadTimeCounter;
        return true;
    }

    std::vector<Tile*> visibleTiles = gameMap.visibleTiles(coveredTiles[0],
            range);
    std::vector<AttackableObject*> enemyObjects = gameMap.getVisibleForce(
            visibleTiles, getColor(), true);

    if (enemyObjects.size() > 0)
    {
        // Select an enemy to shoot at.
        AttackableObject *targetEnemy = enemyObjects[randomUint(0,
                enemyObjects.size() - 1)];
        std::cout << "\nCannon firing. Doing damage to enemy: "
                << targetEnemy->getName();

        // Shoot the targeted enemy.
        targetEnemy->takeDamage(randomDouble(minDamage, maxDamage),
                targetEnemy->getCoveredTiles()[0]);

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

