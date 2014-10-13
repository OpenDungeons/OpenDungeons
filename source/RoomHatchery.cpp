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

#include "RoomHatchery.h"

#include "Tile.h"
#include "GameMap.h"
#include "ChickenEntity.h"
#include "LogManager.h"

#include <Random.h>

const double CHICKEN_SPEED = 0.4;

RoomHatchery::RoomHatchery(GameMap* gameMap) :
    Room(gameMap),
    mSpawnChickenCooldown(0)
{
    setMeshName("Farm");
}

RoomObject* RoomHatchery::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    // We add chicken coops on center tiles only
    if(place == ActiveSpotPlace::activeSpotCenter)
        return loadRoomObject(getGameMap(), "ChickenCoop", tile, 0.0);

    return NULL;
}

void RoomHatchery::notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
{
    if(place == ActiveSpotPlace::activeSpotCenter)
    {
        // We remove the chicken coop
        removeRoomObject(tile);
    }
}

uint32_t RoomHatchery::getNbChickens()
{
    uint32_t nbChickens = 0;
    for(std::vector<Tile*>::iterator it = mCoveredTiles.begin(); it != mCoveredTiles.end(); ++it)
    {
        Tile* tile = *it;
        const std::vector<ChickenEntity*>& chickens = tile->getChickenEntities();
        nbChickens += chickens.size();
    }

    return nbChickens;
}

void RoomHatchery::doUpkeep()
{
    Room::doUpkeep();

    if(mCoveredTiles.empty())
        return;

    uint32_t nbChickens = getNbChickens();
    if(nbChickens < mNumActiveSpots)
    {
        // Chickens have been eaten. We check when we will spawn another one
        ++mSpawnChickenCooldown;
        if(mSpawnChickenCooldown >= 10)
        {
            Tile* tile = mCoveredTiles[Random::Uint(0, mCoveredTiles.size() - 1)];
            ChickenEntity* chicken = new ChickenEntity(getGameMap(), getName());
            Ogre::Vector3 pos(static_cast<Ogre::Real>(tile->x), static_cast<Ogre::Real>(tile->y), 0.0f);
            chicken->setPosition(pos);
            tile->addChickenEntity(chicken);
            getGameMap()->addRoomObject(chicken);
            chicken->setMoveSpeed(CHICKEN_SPEED);
            mSpawnChickenCooldown = 0;
        }
    }
    else
        mSpawnChickenCooldown = 0;
}

bool RoomHatchery::hasOpenCreatureSpot(Creature* c)
{
    return mNumActiveSpots > mCreaturesUsingRoom.size();
}
