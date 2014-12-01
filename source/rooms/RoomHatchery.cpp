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

#include "rooms/RoomHatchery.h"
#include "entities/Tile.h"
#include "gamemap/GameMap.h"
#include "entities/ChickenEntity.h"
#include "utils/ConfigManager.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

const double CHICKEN_SPEED = 0.4;

RoomHatchery::RoomHatchery(GameMap* gameMap) :
    Room(gameMap),
    mSpawnChickenCooldown(0)
{
    setMeshName("Farm");
}

RenderedMovableEntity* RoomHatchery::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    // We add chicken coops on center tiles only
    if(place == ActiveSpotPlace::activeSpotCenter)
        return loadBuildingObject(getGameMap(), "ChickenCoop", tile, 0.0, false);

    return NULL;
}

void RoomHatchery::notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
{
    if(place == ActiveSpotPlace::activeSpotCenter)
    {
        // We remove the chicken coop
        removeBuildingObject(tile);
    }
}

uint32_t RoomHatchery::getNbChickens()
{
    std::vector<GameEntity*> chickens;
    for(Tile* tile : mCoveredTiles)
    {
        tile->fillWithChickenEntities(chickens);
    }

    return chickens.size();
}

void RoomHatchery::doUpkeep()
{
    Room::doUpkeep();

    if(mCoveredTiles.empty())
        return;

    uint32_t nbChickens = getNbChickens();
    if(nbChickens >= mNumActiveSpots)
        return;

    // Chickens have been eaten. We check when we will spawn another one
    ++mSpawnChickenCooldown;
    if(mSpawnChickenCooldown < ConfigManager::getSingleton().getRoomConfigUInt32("HatcheryChickenSpawnRate"))
        return;

    // We spawn 1 chicken per chicken coop (until chickens are maxed)
    for(Tile* chickenCoopTile : mCentralActiveSpotTiles)
    {
        ChickenEntity* chicken = new ChickenEntity(getGameMap(), getName());
        Ogre::Vector3 pos(static_cast<Ogre::Real>(chickenCoopTile->x), static_cast<Ogre::Real>(chickenCoopTile->y), 0.0f);
        chicken->setPosition(pos);
        chickenCoopTile->addChickenEntity(chicken);
        getGameMap()->addRenderedMovableEntity(chicken);
        chicken->setMoveSpeed(CHICKEN_SPEED);
        ++nbChickens;
        if(nbChickens >= mNumActiveSpots)
            break;
    }

    mSpawnChickenCooldown = 0;
}

bool RoomHatchery::hasOpenCreatureSpot(Creature* c)
{
    return mNumActiveSpots > mCreaturesUsingRoom.size();
}
