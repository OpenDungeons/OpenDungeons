#include "Tile.h"
#include "GameMap.h"
#include "TrapBoulder.h"
#include "MissileObject.h"

TrapBoulder::TrapBoulder(int x, int y) :
        DirectionalTrap(x, y)
{
    reloadTime = -1;
    reloadTimeCounter = reloadTime;
    minDamage = 30;
    maxDamage = 40;
}

std::vector<AttackableEntity*> TrapBoulder::aimEnemy() 
{
    std::list<Tile*> tmp = gameMap->lineOfSight(coveredTiles[0]->x,
            coveredTiles[0]->y, dir.first, dir.second);
    std::vector<Tile*> visibleTiles;
    for(std::list<Tile*>::const_iterator it = tmp.begin(); it != tmp.end(); ++it)
    {
        if((*it)->getTilePassability() == Tile::impassableTile)
        {
            break;
        }
        visibleTiles.push_back(*it);
    }
    //By defaut, you damage every attackable object in the line.
    std::vector<AttackableEntity*> v1 = gameMap->getVisibleForce(visibleTiles,
            getColor(), true);
    std::vector<AttackableEntity*> v2 = gameMap->getVisibleForce(visibleTiles,
            getColor(), false); // we also attack our creatures
    for(std::vector<AttackableEntity*>::const_iterator it = v2.begin();
            it != v2.end(); ++it)
    {
        v1.push_back(*it);
    }
    return v1;
}

void TrapBoulder::damage(std::vector<AttackableEntity*> enemyAttacked) // we launch a boulder AND damage creatures, in the futur, the missileobject will be in charge of damaging
{
	DirectionalTrap::damage(enemyAttacked);
	
	if(enemyAttacked.empty())
		return;
	
    // when a Boulder.mesh file exists, do this:
	//~ std::cout << "\nAdding boudler from " << coveredTiles[0]->x << "," << coveredTiles[0]->y << " to " << enemyAttacked.back()->getCoveredTiles()[0]->x << "," << enemyAttacked.back()->getCoveredTiles()[0]->y << std::endl;
	//~ // Create the cannonball to move toward the enemy creature.
	//~ MissileObject *tempMissileObject = new MissileObject("Boulder", Ogre::Vector3(coveredTiles[0]->x, coveredTiles[0]->y, 1));
	//~ tempMissileObject->setMoveSpeed(1.0);
	//~ tempMissileObject->createMesh();
	//~ tempMissileObject->addDestination(enemyAttacked.back()->getCoveredTiles()[0]->x, enemyAttacked.back()->getCoveredTiles()[0]->y, 1);
	//~ gameMap->addMissileObject(tempMissileObject);
}
