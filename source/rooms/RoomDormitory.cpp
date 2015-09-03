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

#include "rooms/RoomDormitory.h"

#include "entities/Tile.h"
#include "game/Player.h"
#include "gamemap/GameMap.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "rooms/RoomManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

static RoomManagerRegister<RoomDormitory> reg(RoomType::dormitory, "Dormitory", "Dormitory room");

RoomDormitory::RoomDormitory(GameMap* gameMap) :
    Room(gameMap)
{
    setMeshName("Dormitory");
}

void RoomDormitory::absorbRoom(Room *r)
{
    if(r->getType() != getType())
    {
        OD_LOG_ERR("Trying to merge incompatible rooms: " + getName() + ", type=" + RoomManager::getRoomNameFromRoomType(getType()) + ", with " + r->getName() + ", type=" + RoomManager::getRoomNameFromRoomType(r->getType()));
        return;
    }

    // We transfert the building objects
    RoomDormitory* oldRoom = static_cast<RoomDormitory*>(r);
    mBedRoomObjectsInfo.insert(mBedRoomObjectsInfo.end(),
        oldRoom->mBedRoomObjectsInfo.begin(), oldRoom->mBedRoomObjectsInfo.end());
    oldRoom->mBedRoomObjectsInfo.clear();

    // This function will copy the building objects (beds) into the new room
    // and remove the old room covered tiles.
    Room::absorbRoom(r);
}

bool RoomDormitory::removeCoveredTile(Tile* t)
{
    OD_ASSERT_TRUE(t != nullptr);
    if (t == nullptr)
        return false;

    if(mTileData.count(t) <= 0)
    {
        OD_LOG_ERR("room=" + getName() + ", tile=" + Tile::displayAsString(t));
        return false;
    }

    RoomDormitoryTileData* roomDormitoryTileData = static_cast<RoomDormitoryTileData*>(mTileData[t]);
    if(roomDormitoryTileData->mCreature != nullptr)
    {
        // Inform the creature that it no longer has a place to sleep
        // and remove the bed tile.
        releaseTileForSleeping(t, roomDormitoryTileData->mCreature);
        roomDormitoryTileData->mCreature = nullptr;
    }

    if(Room::removeCoveredTile(t))
        return true;

    return false;
}

std::vector<Tile*> RoomDormitory::getOpenTiles()
{
    std::vector<Tile*> returnVector;

    for (std::pair<Tile* const, TileData*>& p : mTileData)
    {
        RoomDormitoryTileData* roomDormitoryTileData = static_cast<RoomDormitoryTileData*>(p.second);
        if (roomDormitoryTileData->mCreature == nullptr)
            returnVector.push_back(p.first);
    }

    return returnVector;
}

bool RoomDormitory::claimTileForSleeping(Tile* t, Creature* c)
{
    if (t == nullptr || c == nullptr)
        return false;

    // Check to see if there is already a creature which has claimed this tile for sleeping.
    RoomDormitoryTileData* roomDormitoryTileData = static_cast<RoomDormitoryTileData*>(mTileData[t]);
    if (roomDormitoryTileData->mCreature != nullptr)
        return false;

    double rotationAngle = 0.0;

    // Check to see whether the bed should be situated x-by-y or y-by-x tiles.
    if (tileCanAcceptBed(t, c->getDefinition()->getBedDim1(), c->getDefinition()->getBedDim2()))
        rotationAngle = 0.0;
    else if (tileCanAcceptBed(t, c->getDefinition()->getBedDim2(), c->getDefinition()->getBedDim1()))
        rotationAngle = 90.0;
    else
        return false;

    createBed(t, rotationAngle, c);
    return true;
}

void RoomDormitory::createBed(Tile* t, double rotationAngle, Creature* c)
{
    double xDim = (rotationAngle == 0.0 ? c->getDefinition()->getBedDim1() : c->getDefinition()->getBedDim2());
    double yDim = (rotationAngle == 0.0 ? c->getDefinition()->getBedDim2() : c->getDefinition()->getBedDim1());

    BedRoomObjectInfo bedInfo(static_cast<double>(t->getX()) + xDim / 2.0 - 0.5,
        static_cast<double>(t->getY()) + yDim / 2.0 - 0.5, rotationAngle, c, t);

    // Mark all of the affected tiles as having this creature sleeping in them.
    for (int i = 0; i < xDim; ++i)
    {
        for (int j = 0; j < yDim; ++j)
        {
            Tile *tempTile = getGameMap()->getTile(t->getX() + i, t->getY() + j);
            RoomDormitoryTileData* roomDormitoryTileData = static_cast<RoomDormitoryTileData*>(mTileData[tempTile]);
            roomDormitoryTileData->mCreature = c;
            bedInfo.addTileTaken(tempTile);
        }
    }

    // Add the model
    RenderedMovableEntity* ro = loadBuildingObject(getGameMap(), c->getDefinition()->getBedMeshName(), t, bedInfo.getX(), bedInfo.getY(), rotationAngle, false);
    addBuildingObject(t, ro);
    ro->createMesh();
    // Save the info for later...
    mBedRoomObjectsInfo.push_back(bedInfo);
}

bool RoomDormitory::releaseTileForSleeping(Tile* t, Creature* c)
{
    OD_ASSERT_TRUE(c != nullptr);
    if (c == nullptr)
        return false;

    // Loop over all the tiles in this room and if they are slept on by creature c then set them back to nullptr.
    for (std::pair<Tile* const, TileData*>& p : mTileData)
    {
        RoomDormitoryTileData* roomDormitoryTileData = static_cast<RoomDormitoryTileData*>(p.second);
        if (roomDormitoryTileData->mCreature == c)
            roomDormitoryTileData->mCreature = nullptr;
    }

    Tile* homeTile = c->getHomeTile();
    if(homeTile == nullptr)
    {
        OD_LOG_ERR("creatureName=" + c->getName());
        return false;
    }
    c->setHomeTile(nullptr);

    // Make the building object delete itself and remove it from the map
    RenderedMovableEntity* roomObject = getBuildingObjectFromTile(homeTile);
    removeBuildingObject(roomObject);

    // Remove the bedinfo as well
    for (std::vector<BedRoomObjectInfo>::iterator it = mBedRoomObjectsInfo.begin(); it != mBedRoomObjectsInfo.end();)
    {
        BedRoomObjectInfo& infos = *it;
        if (infos.getOwningTile() == homeTile)
            it = mBedRoomObjectsInfo.erase(it);
        else
            ++it;
    }

    return true;
}

Tile* RoomDormitory::getLocationForBed(int xDim, int yDim)
{
    // Force the dimensions to be positive.
    if (xDim < 0)
        xDim *= -1;
    if (yDim < 0)
        yDim *= -1;

    // Check to see if there is even enough space available for the bed.
    std::vector<Tile*> tempVector = getOpenTiles();
    unsigned int area = xDim * yDim;
    if (tempVector.size() < area)
        return nullptr;

    // Randomly shuffle the open tiles in tempVector so that the dormitory are filled up in a random order.
    std::random_shuffle(tempVector.begin(), tempVector.end());

    // Loop over each of the open tiles in tempVector and for each one, check to see if it
    for (unsigned int i = 0; i < tempVector.size(); ++i)
    {
        if (tileCanAcceptBed(tempVector[i], xDim, yDim))
            return tempVector[i];
    }

    // We got to the end of the open tile list without finding an open tile for the bed so return nullptr to indicate failure.
    return nullptr;
}

bool RoomDormitory::tileCanAcceptBed(Tile *tile, int xDim, int yDim)
{
    //TODO: This function could be made more efficient by making it take the list of open tiles as an argument so if it is called repeatedly the tempTiles vecotor below only has to be computed once in the calling function rather than N times in this function.

    // Force the dimensions to be positive.
    if (xDim < 0)
        xDim *= -1;
    if (yDim < 0)
        yDim *= -1;

    // If either of the dimensions is 0 just return true, since the bed takes no space.  This should never really happen anyway.
    if (xDim == 0 || yDim == 0)
        return true;

    // If the tile is invalid or not part of this room then the bed cannot be placed in this room.
    if (tile == nullptr || tile->getCoveringBuilding() != this)
        return false;

    // Create a 2 dimensional array of booleans initially all set to false.
    std::vector<std::vector<bool> > tileOpen(xDim);
    for (int i = 0; i < xDim; ++i)
    {
        tileOpen[i].resize(yDim, false);
    }

    // Now loop over the list of all the open tiles in this dormitory.  For each tile, if it falls within
    // the xDim by yDim area from the starting tile we set the corresponding tileOpen entry to true.
    std::vector<Tile*> tempTiles = getOpenTiles();
    for (unsigned int i = 0; i < tempTiles.size(); ++i)
    {
        int xDist = tempTiles[i]->getX() - tile->getX();
        int yDist = tempTiles[i]->getY() - tile->getY();
        if (xDist >= 0 && xDist < xDim && yDist >= 0 && yDist < yDim)
            tileOpen[xDist][yDist] = true;
    }

    // Loop over the tileOpen array and check to see if every value has been set to true, if it has then
    // we can place the a bed of the specified dimensions with its corner at the specified starting tile.
    bool returnValue = true;
    for (int i = 0; i < xDim; ++i)
    {
        for (int j = 0; j < yDim; ++j)
        {
            returnValue = returnValue && tileOpen[i][j];
        }
    }

    return returnValue;
}

void RoomDormitory::exportToStream(std::ostream& os) const
{
    Room::exportToStream(os);
    uint32_t nbBeds = mBedRoomObjectsInfo.size();
    os << nbBeds << "\n";
    for(const BedRoomObjectInfo& bed : mBedRoomObjectsInfo)
    {
        os << bed.getCreature()->getName() << "\t";
        os << bed.getOwningTile()->getX() << "\t";
        os << bed.getOwningTile()->getY() << "\t";
        os << bed.getRotation() << "\n";
    }
}

void RoomDormitory::importFromStream(std::istream& is)
{
    Room::importFromStream(is);
    uint32_t nbBeds;
    OD_ASSERT_TRUE(is >> nbBeds);
    while(nbBeds > 0)
    {
        std::string creatureName;
        int x, y;
        double rotation;
        OD_ASSERT_TRUE(is >> creatureName);
        OD_ASSERT_TRUE(is >> x);
        OD_ASSERT_TRUE(is >> y);
        OD_ASSERT_TRUE(is >> rotation);
        mBedCreatureLoad.push_back(BedCreatureLoad(creatureName, x,y, rotation));
        nbBeds--;
    }
}

void RoomDormitory::restoreInitialEntityState()
{
    for(BedCreatureLoad& bedLoad : mBedCreatureLoad)
    {
        Creature* creature = getGameMap()->getCreature(bedLoad.getCreatureName());
        if(creature == nullptr)
        {
            OD_LOG_ERR("creatureName=" + bedLoad.getCreatureName());
            continue;
        }
        Tile* tile = getGameMap()->getTile(bedLoad.getTileX(), bedLoad.getTileY());
        if(creature == nullptr)
        {
            OD_LOG_ERR("tile x=" + Helper::toString(bedLoad.getTileX())
                + ", y=" + Helper::toString(bedLoad.getTileY()));
            continue;
        }
        createBed(tile, bedLoad.getRotationAngle(), creature);
        creature->setHomeTile(tile);
    }
}

RoomDormitoryTileData* RoomDormitory::createTileData(Tile* tile)
{
    return new RoomDormitoryTileData;
}

void RoomDormitory::checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    checkBuildRoomDefault(gameMap, RoomType::dormitory, inputManager, inputCommand);
}

bool RoomDormitory::buildRoom(GameMap* gameMap, Player* player, ODPacket& packet)
{
    std::vector<Tile*> tiles;
    if(!getRoomTilesDefault(tiles, gameMap, player, packet))
        return false;

    int32_t pricePerTarget = RoomManager::costPerTile(RoomType::dormitory);
    int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
    if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
        return false;

    RoomDormitory* room = new RoomDormitory(gameMap);
    return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
}

void RoomDormitory::checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    checkBuildRoomDefaultEditor(gameMap, RoomType::dormitory, inputManager, inputCommand);
}

bool RoomDormitory::buildRoomEditor(GameMap* gameMap, ODPacket& packet)
{
    RoomDormitory* room = new RoomDormitory(gameMap);
    return buildRoomDefaultEditor(gameMap, room, packet);
}

bool RoomDormitory::buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles)
{
    int32_t pricePerTarget = RoomManager::costPerTile(RoomType::dormitory);
    int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
    if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
        return false;

    RoomDormitory* room = new RoomDormitory(gameMap);
    return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
}

bool RoomDormitory::hasCarryEntitySpot(GameEntity* carriedEntity)
{
    if(carriedEntity->getObjectType() != GameEntityType::creature)
        return false;

    if(carriedEntity->getSeat() != getSeat())
        return false;

    Creature* creature = static_cast<Creature*>(carriedEntity);
    if(!creature->canBeCarriedToBuilding(this))
        return false;

    return true;
}

Tile* RoomDormitory::askSpotForCarriedEntity(GameEntity* carriedEntity)
{
    if(carriedEntity->getObjectType() != GameEntityType::creature)
    {
        OD_LOG_ERR("room=" + getName() + ", entity=" + carriedEntity->getName());
        return nullptr;
    }

    Creature* creature = static_cast<Creature*>(carriedEntity);
    // We search for the creature bed
    for(const BedRoomObjectInfo& bed : mBedRoomObjectsInfo)
    {
        if(bed.getCreature() == creature)
            return bed.getOwningTile();
    }

    return nullptr;
}

void RoomDormitory::notifyCarryingStateChanged(Creature* carrier, GameEntity* carriedEntity)
{
    if(carriedEntity->getObjectType() != GameEntityType::creature)
    {
        OD_LOG_ERR("room=" + getName() + ", entity=" + carriedEntity->getName());
        return;
    }

    Creature* creature = static_cast<Creature*>(carriedEntity);
    Tile* posTile = creature->getPositionTile();
    if(posTile == nullptr)
    {
        OD_LOG_ERR("creature=" + creature->getName() + ", position=" + Helper::toString(creature->getPosition()));
        return;
    }

    if(posTile != creature->getHomeTile())
        return;

    creature->releasedInBed();
}

Room* RoomDormitory::getRoomFromStream(GameMap* gameMap, std::istream& is)
{
    RoomDormitory* room = new RoomDormitory(gameMap);
    room->importFromStream(is);
    return room;
}
