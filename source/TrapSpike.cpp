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

#include "TrapSpike.h"

#include "Tile.h"
#include "GameMap.h"
#include "MissileObject.h"
#include "Random.h"
#include "RoomObject.h"
#include "LogManager.h"

TrapSpike::TrapSpike(GameMap* gameMap) :
    ProximityTrap(gameMap)
{
    mReloadTime = 5;
    mRange = 10;
    mMinDamage = 10;
    mMaxDamage = 15;
}

bool TrapSpike::shoot(Tile* tile)
{
    std::vector<Tile*> visibleTiles;
    visibleTiles.push_back(tile);
    std::vector<GameEntity*> enemyObjects = getGameMap()->getVisibleForce(visibleTiles, getSeat(), true);
    if(enemyObjects.empty())
        return false;

    RoomObject* spike = getRoomObjectFromTile(tile);
    spike->setAnimationState("Triggered", false);

    // We damage every creature standing on the trap
    for(std::vector<GameEntity*>::iterator it = enemyObjects.begin(); it != enemyObjects.end(); ++it)
    {
        GameEntity* target = *it;
        if(target->getObjectType() != GameEntity::ObjectType::creature)
            continue;

        target->takeDamage(this, Random::Double(mMinDamage, mMaxDamage), target->getCoveredTiles()[0]);
    }
    std::vector<GameEntity*> alliedObjects = getGameMap()->getVisibleForce(visibleTiles, getSeat(), false);
    for(std::vector<GameEntity*>::iterator it = alliedObjects.begin(); it != alliedObjects.end(); ++it)
    {
        GameEntity* target = *it;
        if(target->getObjectType() != GameEntity::ObjectType::creature)
            continue;

        target->takeDamage(this, Random::Double(mMinDamage, mMaxDamage), target->getCoveredTiles()[0]);
    }
    return true;
}


RoomObject* TrapSpike::notifyActiveSpotCreated(Tile* tile)
{
    return loadRoomObject(getGameMap(), "Spiketrap", tile, 0.0);
}
