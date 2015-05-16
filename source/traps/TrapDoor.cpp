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

#include "traps/TrapDoor.h"

#include "entities/DoorEntity.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/Tile.h"
#include "entities/TrapEntity.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "traps/TrapManager.h"
#include "utils/ConfigManager.h"
#include "utils/Random.h"
#include "utils/LogManager.h"

static TrapManagerRegister<TrapDoor> reg(TrapType::doorWooden, "DoorWooden");

const std::string TrapDoor::MESH_DOOR = "TentacleAlbine";

TrapDoor::TrapDoor(GameMap* gameMap) :
    Trap(gameMap),
    mIsLocked(false)
{
    mReloadTime = 0;
    mMinDamage = 0;
    mMaxDamage = 0;
    mNbShootsBeforeDeactivation = 1;
    setMeshName("Spike");
}

TrapEntity* TrapDoor::getTrapEntity(Tile* tile)
{
    return new DoorEntity(getGameMap(), getSeat(), getName(), MESH_DOOR, tile, 0.0, true, isActivated(tile) ? 1.0f : 0.7f);
}

bool TrapDoor::isAttackable(Tile* tile, Seat* seat) const
{
    if(getSeat()->isAlliedSeat(seat))
        return false;

    return true;
}

void TrapDoor::activate(Tile* tile)
{
    Trap::activate(tile);
    getGameMap()->doorLock(tile, getSeat(), mIsLocked);
}

void TrapDoor::doUpkeep()
{
    for(Tile* tile : mCoveredTiles)
    {
        if(!canDoorBeOnTile(getGameMap(), tile))
        {
            if(mTileData.count(tile) <= 0)
            {
                OD_ASSERT_TRUE_MSG(false, "trap=" + getName() + ", tile=" + Tile::displayAsString(tile));
                return;
            }

            TrapTileData* trapTileData = static_cast<TrapTileData*>(mTileData.at(tile));
            trapTileData->mHP = 0.0;
        }

        // We need to look for destroyed door before calling Trap::doUpkeep otherwise, they will be removed
        // from covered tiles
        if (mTileData[tile]->mHP <= 0.0)
        {
            getGameMap()->doorLock(tile, getSeat(), false);
        }
    }

    Trap::doUpkeep();
}

void TrapDoor::notifyDoorSlapped(Tile* tile)
{
     mIsLocked = !mIsLocked;
    if(!isActivated(tile))
        return;

     getGameMap()->doorLock(tile, getSeat(), mIsLocked);
}

bool TrapDoor::canDoorBeOnTile(GameMap* gameMap, Tile* tile)
{
    // We check if the tile is suitable. It can only be built on 2 full tiles
    Tile* tileW = gameMap->getTile(tile->getX() - 1, tile->getY());
    Tile* tileE = gameMap->getTile(tile->getX() + 1, tile->getY());

    if((tileW != nullptr) &&
       (tileE != nullptr) &&
       tileW->isFullTile() &&
       tileE->isFullTile())
    {
        // Ok
        return true;
    }

    Tile* tileS = gameMap->getTile(tile->getX(), tile->getY() - 1);
    Tile* tileN = gameMap->getTile(tile->getX(), tile->getY() + 1);
    if((tileS != nullptr) &&
       (tileN != nullptr) &&
       tileS->isFullTile() &&
       tileN->isFullTile())
    {
        // Ok
        return true;
    }

    return false;
}

TrapDoor* TrapDoor::getTrapDoorFromStream(GameMap* gameMap, std::istream &is)
{
    TrapDoor* trap = new TrapDoor(gameMap);
    return trap;
}

TrapDoor* TrapDoor::getTrapDoorFromPacket(GameMap* gameMap, ODPacket &is)
{
    TrapDoor* trap = new TrapDoor(gameMap);
    return trap;
}

int TrapDoor::getTrapCost(std::vector<Tile*>& tiles, GameMap* gameMap, TrapType type,
    int tileX1, int tileY1, int tileX2, int tileY2, Player* player)
{
    std::vector<Tile*> buildableTiles = gameMap->getBuildableTilesForPlayerInArea(tileX1,
        tileY1, tileX2, tileY2, player);

    if(buildableTiles.empty())
        return TrapManager::costPerTile(type);

    // We cannot build more than 1 door at a time. If more than 1 buildable tile is selected, we consider
    // it is a problem
    if(buildableTiles.size() > 1)
        return -1;

    Tile* tile = buildableTiles[0];
    if(canDoorBeOnTile(gameMap, tile))
    {
        tiles.push_back(tile);
        return TrapManager::costPerTile(type);
    }

    return -1;
}

void TrapDoor::buildTrap(GameMap* gameMap, const std::vector<Tile*>& tiles, Seat* seat)
{
    TrapDoor* room = new TrapDoor(gameMap);
    buildTrapDefault(gameMap, room, tiles, seat);
}

Trap* TrapDoor::getTrapFromStream(GameMap* gameMap, std::istream& is)
{
    return new TrapDoor(gameMap);
}
