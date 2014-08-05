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

#include "TrapCannon.h"

#include "Tile.h"
#include "GameMap.h"
#include "MissileObject.h"
#include "Random.h"

TrapCannon::TrapCannon(GameMap* gameMap) :
    ProximityTrap(gameMap),
    mCannonHeight(1.5)
{
    mReloadTime = 5;
    mReloadTimeCounter = mReloadTime;
    mRange = 12;
    mMinDamage = 104;
    mMaxDamage = 120;
}

std::vector<GameEntity*> TrapCannon::aimEnemy()
{
    std::vector<Tile*> visibleTiles = getGameMap()->visibleTiles(mCoveredTiles[0], mRange);
    std::vector<GameEntity*> enemyObjects = getGameMap()->getVisibleForce(visibleTiles, getColor(), true);

    if(enemyObjects.empty())
    {
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

    std::cout << "\nAdding cannonball from " << mCoveredTiles[0]->x << "," << mCoveredTiles[0]->y
        << " to " << enemyAttacked[0]->getCoveredTiles()[0]->x << "," << enemyAttacked[0]->getCoveredTiles()[0]->y << std::endl;

    // Create the cannonball to move toward the enemy creature.
    MissileObject *tempMissileObject = new MissileObject(getGameMap(),
        "Cannonball", Ogre::Vector3((Ogre::Real)mCoveredTiles[0]->x, (Ogre::Real)mCoveredTiles[0]->y,
                                    (Ogre::Real)mCannonHeight));

    tempMissileObject->setMoveSpeed(8.0);
    tempMissileObject->createMesh();
    //TODO: Make this a pseudo newtonian mechanics solver which computes a parabola passing through the cannon
    // and the enemy it is shooting at, add this as 10 or so destinations in the queue instead of just one.
    getGameMap()->addMissileObject(tempMissileObject);
    tempMissileObject->addDestination((Ogre::Real)enemyAttacked[0]->getCoveredTiles()[0]->x,
                                    (Ogre::Real)enemyAttacked[0]->getCoveredTiles()[0]->y,
                                    (Ogre::Real)mCannonHeight);
}
