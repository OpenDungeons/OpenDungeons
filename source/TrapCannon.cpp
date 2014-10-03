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
#include "LogManager.h"

TrapCannon::TrapCannon(GameMap* gameMap) :
    ProximityTrap(gameMap),
    mCannonHeight(1.5)
{
    mReloadTime = 5;
    mRange = 10;
    mMinDamage = 5;
    mMaxDamage = 10;
}

bool TrapCannon::shoot(Tile* tile)
{
    std::vector<Tile*> visibleTiles = getGameMap()->visibleTiles(tile, mRange);
    std::vector<GameEntity*> enemyObjects = getGameMap()->getVisibleForce(visibleTiles, getSeat(), true);

    if(enemyObjects.empty())
        return false;

    // Select an enemy to shoot at.
    GameEntity* targetEnemy = enemyObjects[Random::Uint(0, enemyObjects.size()-1)];

    // TODO : instead of dealing damages here, add to MissileObject the needed infos to make it damage the
    // target when hit (to allow pickup before getting hurt or dodging)
    targetEnemy->takeDamage(this, Random::Double(mMinDamage, mMaxDamage), targetEnemy->getCoveredTiles()[0]);
    // Create the cannonball to move toward the enemy creature.
    MissileObject *tempMissileObject = new MissileObject(getGameMap(),
        "Cannonball", Ogre::Vector3((Ogre::Real)tile->x, (Ogre::Real)tile->y,
                                    (Ogre::Real)mCannonHeight));

    //TODO: Make this a pseudo newtonian mechanics solver which computes a parabola passing through the cannon
    // and the enemy it is shooting at, add this as 10 or so destinations in the queue instead of just one.
    getGameMap()->addMissileObject(tempMissileObject);
    tempMissileObject->setMoveSpeed(8.0);
    tempMissileObject->createMesh();
    tempMissileObject->addDestination((Ogre::Real)targetEnemy->getCoveredTiles()[0]->x,
                                    (Ogre::Real)targetEnemy->getCoveredTiles()[0]->y,
                                    (Ogre::Real)mCannonHeight);
    return true;
}

RoomObject* TrapCannon::notifyActiveSpotCreated(Tile* tile)
{
    return loadRoomObject(getGameMap(), "Cannon", tile, 90.0);
}
