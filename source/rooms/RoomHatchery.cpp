/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
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
#include "entities/ChickenEntity.h"
#include "game/Player.h"
#include "gamemap/GameMap.h"
#include "rooms/RoomManager.h"
#include "utils/ConfigManager.h"
#include "utils/LogManager.h"

static RoomManagerRegister<RoomHatchery> reg(RoomType::hatchery, "Hatchery");

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

    return nullptr;
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
        ChickenEntity* chicken = new ChickenEntity(getGameMap(), true, getName());
        chicken->addToGameMap();
        chicken->createMesh();
        Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(chickenCoopTile->getX()),
                                    static_cast<Ogre::Real>(chickenCoopTile->getY()), 0.0f);
        chicken->setPosition(spawnPosition);
        chicken->setMoveSpeed(CHICKEN_SPEED, 1.0);
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

void RoomHatchery::checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    checkBuildRoomDefault(gameMap, RoomType::hatchery, inputManager, inputCommand);
}

bool RoomHatchery::buildRoom(GameMap* gameMap, Player* player, ODPacket& packet)
{
    std::vector<Tile*> tiles;
    if(!getRoomTilesDefault(tiles, gameMap, player, packet))
        return false;

    int32_t pricePerTarget = RoomManager::costPerTile(RoomType::hatchery);
    int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
    if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
        return false;

    RoomHatchery* room = new RoomHatchery(gameMap);
    return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
}

bool RoomHatchery::buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles)
{
    int32_t pricePerTarget = RoomManager::costPerTile(RoomType::hatchery);
    int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
    if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
        return false;

    RoomHatchery* room = new RoomHatchery(gameMap);
    return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
}

void RoomHatchery::checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    checkBuildRoomDefaultEditor(gameMap, RoomType::hatchery, inputManager, inputCommand);
}

bool RoomHatchery::buildRoomEditor(GameMap* gameMap, ODPacket& packet)
{
    RoomHatchery* room = new RoomHatchery(gameMap);
    return buildRoomDefaultEditor(gameMap, room, packet);
}

Room* RoomHatchery::getRoomFromStream(GameMap* gameMap, std::istream& is)
{
    return new RoomHatchery(gameMap);
}
