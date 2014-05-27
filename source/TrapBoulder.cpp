/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TrapBoulder.h"

#include "Tile.h"
#include "GameMap.h"
#include "MissileObject.h"

TrapBoulder::TrapBoulder(int x, int y) :
    DirectionalTrap(x, y)
{
    mReloadTime = -1;
    mReloadTimeCounter = mReloadTime;
    mMinDamage = 30;
    mMaxDamage = 40;
}

std::vector<GameEntity*> TrapBoulder::aimEnemy()
{
    std::list<Tile*> tmp = getGameMap()->lineOfSight(mCoveredTiles[0]->x,
            mCoveredTiles[0]->y, mDir.first, mDir.second);
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
    std::vector<GameEntity*> v1 = getGameMap()->getVisibleForce(visibleTiles,
            getColor(), true);
    std::vector<GameEntity*> v2 = getGameMap()->getVisibleForce(visibleTiles,
            getColor(), false); // we also attack our creatures
    for(std::vector<GameEntity*>::const_iterator it = v2.begin();
            it != v2.end(); ++it)
    {
        v1.push_back(*it);
    }

    return v1;
}

// we launch a boulder AND damage creatures, in the futur, the missileobject will be in charge of damaging
void TrapBoulder::damage(std::vector<GameEntity*> enemyAttacked)
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
