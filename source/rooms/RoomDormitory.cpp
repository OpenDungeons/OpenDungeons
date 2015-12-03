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
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

const std::string RoomDormitoryName = "Dormitory";
const std::string RoomDormitoryNameDisplay = "Dormitory room";
const RoomType RoomDormitory::mRoomType = RoomType::dormitory;

namespace
{
class RoomDormitoryFactory : public RoomFactory
{
    RoomType getRoomType() const override
    { return RoomDormitory::mRoomType; }

    const std::string& getName() const override
    { return RoomDormitoryName; }

    const std::string& getNameReadable() const override
    { return RoomDormitoryNameDisplay; }

    int getCostPerTile() const override
    { return ConfigManager::getSingleton().getRoomConfigInt32("DormitoryCostPerTile"); }

    void checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildRoomDefault(gameMap, RoomDormitory::mRoomType, inputManager, inputCommand);
    }

    bool buildRoom(GameMap* gameMap, Player* player, ODPacket& packet) const override
    {
        std::vector<Tile*> tiles;
        if(!getRoomTilesDefault(tiles, gameMap, player, packet))
            return false;

        int32_t pricePerTarget = RoomManager::costPerTile(RoomDormitory::mRoomType);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        RoomDormitory* room = new RoomDormitory(gameMap);
        return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
    }

    void checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildRoomDefaultEditor(gameMap, RoomDormitory::mRoomType, inputManager, inputCommand);
    }

    bool buildRoomEditor(GameMap* gameMap, ODPacket& packet) const override
    {
        RoomDormitory* room = new RoomDormitory(gameMap);
        return buildRoomDefaultEditor(gameMap, room, packet);
    }

    Room* getRoomFromStream(GameMap* gameMap, std::istream& is) const override
    {
        RoomDormitory* room = new RoomDormitory(gameMap);
        if(!Room::importRoomFromStream(*room, is))
        {
            OD_LOG_ERR("Error while building a room from the stream");
        }
        return room;
    }

    bool buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles) const override
    {
        int32_t pricePerTarget = RoomManager::costPerTile(RoomDormitory::mRoomType);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        RoomDormitory* room = new RoomDormitory(gameMap);
        return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
    }
};

// Register the factory
static RoomRegister reg(new RoomDormitoryFactory);
}

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

    auto it = mTileData.find(t);
    if(it == mTileData.end())
    {
        OD_LOG_ERR("room=" + getName() + ", tile=" + Tile::displayAsString(t));
        return false;
    }

    RoomDormitoryTileData* roomDormitoryTileData = static_cast<RoomDormitoryTileData*>(it->second);
    if(roomDormitoryTileData->mCreature != nullptr)
    {
        // Inform the creature that it no longer has a place to sleep
        // and remove the bed tile.
        releaseTileForSleeping(t, roomDormitoryTileData->mCreature);
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
        if (roomDormitoryTileData->mHP <=0)
            continue;
        if (roomDormitoryTileData->mCreature != nullptr)
            continue;

        returnVector.push_back(p.first);
    }

    return returnVector;
}

Tile* RoomDormitory::claimTileForSleeping(Tile* t, Creature* c)
{
    if (t == nullptr || c == nullptr)
        return nullptr;

    // Check to see if there is already a creature which has claimed this tile for sleeping.
    RoomDormitoryTileData* roomDormitoryTileData = static_cast<RoomDormitoryTileData*>(mTileData[t]);
    if (roomDormitoryTileData->mCreature != nullptr)
        return nullptr;

    double rotationAngle = 0.0;

    // Check to see whether the bed should be situated x-by-y or y-by-x tiles.
    int xSleep = t->getX();
    int ySleep = t->getY();
    int width;
    int height;
    Ogre::Vector3 sleepDirection = Ogre::Vector3::ZERO;
    std::vector<Tile*> openTiles = getOpenTiles();
    if (tileCanAcceptBed(t, c->getDefinition()->getBedDim1(), c->getDefinition()->getBedDim2(), openTiles))
    {
        rotationAngle = 0.0;
        width = c->getDefinition()->getBedDim1();
        height = c->getDefinition()->getBedDim2();
        xSleep += (c->getDefinition()->getBedPosX() - 1);
        ySleep += (c->getDefinition()->getBedPosY() - 1);
        sleepDirection.x = c->getDefinition()->getBedOrientX();
        sleepDirection.y = c->getDefinition()->getBedOrientY();
    }
    else if (tileCanAcceptBed(t, c->getDefinition()->getBedDim2(), c->getDefinition()->getBedDim1(), openTiles))
    {
        rotationAngle = 90.0;
        width = c->getDefinition()->getBedDim2();
        height = c->getDefinition()->getBedDim1();
        xSleep += (c->getDefinition()->getBedPosY() - 1);
        ySleep += (c->getDefinition()->getBedPosX() - 1);
        sleepDirection.x = c->getDefinition()->getBedOrientY();
        sleepDirection.y = c->getDefinition()->getBedOrientX();
    }
    else
        return nullptr;

    // We find the tile where the creature is supposed to sleep
    Tile* homeTile = getGameMap()->getTile(xSleep, ySleep);

    createBed(homeTile, t->getX(), t->getY(), width, height, rotationAngle, c, sleepDirection);
    return homeTile;
}

void RoomDormitory::createBed(Tile* sleepTile, int x, int y, int width, int height,
        double rotationAngle, Creature* c, const Ogre::Vector3& sleepDirection)
{
    BedRoomObjectInfo bedInfo(x, y, rotationAngle, c, sleepTile, sleepDirection);

    // Mark all of the affected tiles as having this creature sleeping in them.
    for (int i = 0; i < width; ++i)
    {
        for (int j = 0; j < height; ++j)
        {
            Tile *tempTile = getGameMap()->getTile(x + i, y + j);
            RoomDormitoryTileData* roomDormitoryTileData = static_cast<RoomDormitoryTileData*>(mTileData[tempTile]);
            roomDormitoryTileData->mCreature = c;
            bedInfo.addTileTaken(tempTile);
        }
    }

    // Add the model
    double xMesh = static_cast<double>(x) + (static_cast<double>(width) / 2.0) - 0.5;
    double yMesh = static_cast<double>(y) + (static_cast<double>(height) / 2.0) - 0.5;
    RenderedMovableEntity* ro = loadBuildingObject(getGameMap(), c->getDefinition()->getBedMeshName(), sleepTile, xMesh, yMesh, rotationAngle, false);
    addBuildingObject(sleepTile, ro);
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

Tile* RoomDormitory::getLocationForBed(Creature* creature)
{
    int bedDim1 = creature->getDefinition()->getBedDim1();
    int bedDim2 = creature->getDefinition()->getBedDim2();
    if((bedDim1 <= 0) || (bedDim2 <= 0))
    {
        OD_LOG_ERR("creature=" + creature->getName() + " wrong bed size bedDim1=" + Helper::toString(bedDim1) + ", bedDim2=" + Helper::toString(bedDim2));
        return nullptr;
    }

    for(int i = 0; i < 2; ++i)
    {
        int xDim;
        int yDim;
        // We try the default size and with a rotation
        if(i == 0)
        {
            xDim = bedDim1;
            yDim = bedDim2;
        }
        else
        {
            xDim = bedDim2;
            yDim = bedDim1;
        }

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
            if (tileCanAcceptBed(tempVector[i], xDim, yDim, getOpenTiles()))
                return tempVector[i];
        }
    }

    // We got to the end of the open tile list without finding an open tile for the bed so return nullptr to indicate failure.
    return nullptr;
}

const Ogre::Vector3& RoomDormitory::getSleepDirection(Creature* creature) const
{
    for (const BedRoomObjectInfo& infos : mBedRoomObjectsInfo)
    {
        if(infos.getCreature() != creature)
            continue;

        return infos.getSleepDirection();
    }

    OD_LOG_ERR("room=" + getName() + ", creature=" + creature->getName());
    return Ogre::Vector3::UNIT_X;
}

bool RoomDormitory::tileCanAcceptBed(Tile *tile, int xDim, int yDim, const std::vector<Tile*>& openTiles)
{
    if((xDim <= 0) || (yDim <= 0))
    {
        OD_LOG_ERR("wrong bed size x=" + Helper::toString(xDim) + ", y=" + Helper::toString(yDim));
        return false;
    }

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
    for (Tile* tempTile : openTiles)
    {
        int xDist = tempTile->getX() - tile->getX();
        int yDist = tempTile->getY() - tile->getY();
        if (xDist >= 0 && xDist < xDim && yDist >= 0 && yDist < yDim)
            tileOpen[xDist][yDist] = true;
    }

    // Loop over the tileOpen array and check to see if every value has been set to true, if it has then
    // we can place the a bed of the specified dimensions with its corner at the specified starting tile.
    for (int i = 0; i < xDim; ++i)
    {
        for (int j = 0; j < yDim; ++j)
        {
            if(!tileOpen[i][j])
                return false;
        }
    }

    return true;
}

void RoomDormitory::exportToStream(std::ostream& os) const
{
    Room::exportToStream(os);
    uint32_t nbBeds = mBedRoomObjectsInfo.size();
    os << nbBeds << "\n";
    for(const BedRoomObjectInfo& bed : mBedRoomObjectsInfo)
    {
        // We save the position of the tile bottom left so that if the sleeping
        // position of the creature changes, the savegames are not impacted
        os << bed.getCreature()->getName() << "\t";
        os << bed.getX() << "\t";
        os << bed.getY() << "\t";
        os << bed.getRotation() << "\n";
    }
}

bool RoomDormitory::importFromStream(std::istream& is)
{
    if(!Room::importFromStream(is))
        return false;
    uint32_t nbBeds;
    if(!(is >> nbBeds))
        return false;
    while(nbBeds > 0)
    {
        std::string creatureName;
        int x, y;
        double rotation;
        if(!(is >> creatureName))
            return false;
        if(!(is >> x))
            return false;
        if(!(is >> y))
            return false;
        if(!(is >> rotation))
            return false;
        mBedCreatureLoad.push_back(BedCreatureLoad(creatureName, x,y, rotation));
        nbBeds--;
    }

    return true;
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
        int x = tile->getX();
        int y = tile->getY();
        int width;
        int height;
        Ogre::Vector3 sleepDirection = Ogre::Vector3::ZERO;
        if(bedLoad.getRotationAngle() == 0.0)
        {
            x += creature->getDefinition()->getBedPosX() - 1;
            y += creature->getDefinition()->getBedPosY() - 1;
            width = creature->getDefinition()->getBedDim1();
            height = creature->getDefinition()->getBedDim2();
            sleepDirection.x = creature->getDefinition()->getBedOrientX();
            sleepDirection.y = creature->getDefinition()->getBedOrientY();
        }
        else
        {
            x += creature->getDefinition()->getBedPosY() - 1;
            y += creature->getDefinition()->getBedPosX() - 1;
            width = creature->getDefinition()->getBedDim2();
            height = creature->getDefinition()->getBedDim1();
            sleepDirection.x = creature->getDefinition()->getBedOrientY();
            sleepDirection.y = creature->getDefinition()->getBedOrientX();
        }

        Tile* sleepTile = getGameMap()->getTile(x, y);
        if(sleepTile == nullptr)
        {
            OD_LOG_ERR("creature=" + creature->getName() + ", tile x=" + Helper::toString(x)
                + ", y=" + Helper::toString(y));
            continue;
        }
        createBed(tile, tile->getX(), tile->getY(), width, height, bedLoad.getRotationAngle(), creature, sleepDirection);
        creature->setHomeTile(sleepTile);
    }
}

RoomDormitoryTileData* RoomDormitory::createTileData(Tile* tile)
{
    return new RoomDormitoryTileData;
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
