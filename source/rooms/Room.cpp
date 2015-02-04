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

#include "rooms/Room.h"

#include "rooms/RoomTrainingHall.h"
#include "rooms/RoomDungeonTemple.h"
#include "rooms/RoomForge.h"
#include "rooms/RoomLibrary.h"
#include "rooms/RoomPortal.h"
#include "rooms/RoomDormitory.h"
#include "rooms/RoomTreasury.h"
#include "rooms/RoomHatchery.h"
#include "rooms/RoomCrypt.h"
#include "entities/RenderedMovableEntity.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "game/Player.h"
#include "entities/Tile.h"
#include "entities/Creature.h"
#include "gamemap/GameMap.h"
#include "game/Seat.h"
#include "utils/LogManager.h"

#include <sstream>

Room::Room(GameMap* gameMap):
    Building(gameMap),
    mNumActiveSpots(0)
{
}

bool Room::compareTile(Tile* tile1, Tile* tile2)
{
    if(tile1->getX() < tile2->getX())
        return true;

    if(tile1->getX() == tile2->getX())
        return (tile1->getY() < tile2->getY());

    return false;
}

void Room::absorbRoom(Room *r)
{
    LogManager::getSingleton().logMessage(getGameMap()->serverStr() + "Room=" + getName() + " is aborbing room=" + r->getName());

    mCentralActiveSpotTiles.insert(mCentralActiveSpotTiles.end(), r->mCentralActiveSpotTiles.begin(), r->mCentralActiveSpotTiles.end());
    r->mCentralActiveSpotTiles.clear();
    mLeftWallsActiveSpotTiles.insert(mLeftWallsActiveSpotTiles.end(), r->mLeftWallsActiveSpotTiles.begin(), r->mLeftWallsActiveSpotTiles.end());
    r->mLeftWallsActiveSpotTiles.clear();
    mRightWallsActiveSpotTiles.insert(mRightWallsActiveSpotTiles.end(), r->mRightWallsActiveSpotTiles.begin(), r->mRightWallsActiveSpotTiles.end());
    r->mRightWallsActiveSpotTiles.clear();
    mTopWallsActiveSpotTiles.insert(mTopWallsActiveSpotTiles.end(), r->mTopWallsActiveSpotTiles.begin(), r->mTopWallsActiveSpotTiles.end());
    r->mTopWallsActiveSpotTiles.clear();
    mBottomWallsActiveSpotTiles.insert(mBottomWallsActiveSpotTiles.end(), r->mBottomWallsActiveSpotTiles.begin(), r->mBottomWallsActiveSpotTiles.end());
    r->mBottomWallsActiveSpotTiles.clear();
    mNumActiveSpots += r->mNumActiveSpots;

    // Every creature working in this room should go to the new one (this is used in the server map only)
    if(getGameMap()->isServerGameMap())
    {
        for(Creature* creature : r->mCreaturesUsingRoom)
        {
            if(creature->isJobRoom(r))
                creature->changeJobRoom(this);
            else if(creature->isEatRoom(r))
                creature->changeEatRoom(this);
            else
            {
                OD_ASSERT_TRUE_MSG(false, "creature=" + creature->getName() + ", oldRoom=" + r->getName() + ", newRoom=" + getName());
            }
        }
        mCreaturesUsingRoom.insert(mCreaturesUsingRoom.end(), r->mCreaturesUsingRoom.begin(), r->mCreaturesUsingRoom.end());
        r->mCreaturesUsingRoom.clear();
    }

    mBuildingObjects.insert(r->mBuildingObjects.begin(), r->mBuildingObjects.end());
    r->mBuildingObjects.clear();

    for(Tile* tile : r->mCoveredTiles)
    {
        double hp = r->getHP(tile);
        // We don't want to notify the room for the addition of the tile because it is not a new tile. That's why
        // we call Building::addCoveredTile and not Room::addCoveredTile. If a room wants to handle its ground mesh
        // differently, it can override reorderRoomAfterAbsorbtion.
        Building::addCoveredTile(tile, hp);
    }
    // We don't need to insert r->mTileHP and r->mCoveredTiles because it has already been done in Building::addCoveredTile
    r->mCoveredTiles.clear();
    r->mTileHP.clear();
}

void Room::reorderRoomAfterAbsorbtion()
{
    // We try to keep the same tile disposition as if the room was created like this in the first
    // place to make sure building objects are disposed the same way
    std::sort(mCoveredTiles.begin(), mCoveredTiles.end(), Room::compareTile);
}

bool Room::addCreatureUsingRoom(Creature* c)
{
    if(!hasOpenCreatureSpot(c))
        return false;

    mCreaturesUsingRoom.push_back(c);
    return true;
}

void Room::removeCreatureUsingRoom(Creature *c)
{
    for (unsigned int i = 0; i < mCreaturesUsingRoom.size(); ++i)
    {
        if (mCreaturesUsingRoom[i] == c)
        {
            mCreaturesUsingRoom.erase(mCreaturesUsingRoom.begin() + i);
            break;
        }
    }
}

Creature* Room::getCreatureUsingRoom(unsigned index)
{
    if (index >= mCreaturesUsingRoom.size())
        return nullptr;

    return mCreaturesUsingRoom[index];
}

std::string Room::getFormat()
{
    return "typeRoom\tseatId\tnumTiles\t\tSubsequent Lines: tileX\ttileY";
}

void Room::doUpkeep()
{
    // Loop over the tiles in Room r and remove any whose HP has dropped to zero.
    std::vector<Tile*> tilesToRemove;
    for (Tile* tile : mCoveredTiles)
    {
        if (mTileHP[tile] <= 0.0)
        {
            tilesToRemove.push_back(tile);
            continue;
        }
    }

    if (!tilesToRemove.empty())
    {
        for(Tile* tile : tilesToRemove)
            removeCoveredTile(tile);

        updateActiveSpots();
        createMesh();
    }

    // If no more tiles, the room is removed
    if (numCoveredTiles() <= 0)
    {
        getGameMap()->removeRoom(this);
        deleteYourself();
        return;
    }
}

Room* Room::getRoomFromStream(GameMap* gameMap, std::istream& is)
{
    Room* tempRoom = nullptr;
    RoomType nType;
    is >> nType;

    switch (nType)
    {
        case nullRoomType:
            tempRoom = nullptr;
            break;
        case dormitory:
            tempRoom = new RoomDormitory(gameMap);
            break;
        case treasury:
            tempRoom = new RoomTreasury(gameMap);
            break;
        case portal:
            tempRoom = new RoomPortal(gameMap);
            break;
        case dungeonTemple:
            tempRoom = new RoomDungeonTemple(gameMap);
            break;
        case forge:
            tempRoom = new RoomForge(gameMap);
            break;
        case trainingHall:
            tempRoom = new RoomTrainingHall(gameMap);
            break;
        case library:
            tempRoom = new RoomLibrary(gameMap);
            break;
        case hatchery:
            tempRoom = new RoomHatchery(gameMap);
            break;
        case crypt:
            tempRoom = new RoomCrypt(gameMap);
            break;
        default:
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(nType)));
    }

    if(tempRoom == nullptr)
        return nullptr;

    tempRoom->importFromStream(is);

    return tempRoom;
}

Room* Room::getRoomFromPacket(GameMap* gameMap, ODPacket& is)
{
    Room* tempRoom = nullptr;
    RoomType nType;
    is >> nType;

    switch (nType)
    {
        case nullRoomType:
            tempRoom = nullptr;
            break;
        case dormitory:
            tempRoom = new RoomDormitory(gameMap);
            break;
        case treasury:
            tempRoom = new RoomTreasury(gameMap);
            break;
        case portal:
            tempRoom = new RoomPortal(gameMap);
            break;
        case dungeonTemple:
            tempRoom = new RoomDungeonTemple(gameMap);
            break;
        case forge:
            tempRoom = new RoomForge(gameMap);
            break;
        case trainingHall:
            tempRoom = new RoomTrainingHall(gameMap);
            break;
        case library:
            tempRoom = new RoomLibrary(gameMap);
            break;
        case hatchery:
            tempRoom = new RoomHatchery(gameMap);
            break;
        case crypt:
            tempRoom = new RoomCrypt(gameMap);
            break;
        default:
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(nType)));
    }

    if(tempRoom == nullptr)
        return nullptr;

    tempRoom->importFromPacket(is);

    return tempRoom;
}

const std::string Room::getRoomNameFromRoomType(RoomType t)
{
    switch (t)
    {
    case nullRoomType:
        return "NullRoomType";

    case dungeonTemple:
        return "DungeonTemple";

    case dormitory:
        return "Dormitory";

    case treasury:
        return "Treasury";

    case portal:
        return "Portal";

    case forge:
        return "Forge";

    case trainingHall:
        return "TrainingHall";

    case library:
        return "Library";

    case hatchery:
        return "Hatchery";

    case crypt:
        return "Crypt";

    default:
        return "UnknownRoomType";
    }
}

Room::RoomType Room::getRoomTypeFromRoomName(const std::string& name)
{
    if(name.compare("DungeonTemple") == 0)
        return dungeonTemple;

    if(name.compare("Dormitory") == 0)
        return dormitory;

    if(name.compare("Treasury") == 0)
        return treasury;

    if(name.compare("Portal") == 0)
        return portal;

    if(name.compare("Forge") == 0)
        return forge;

    if(name.compare("TrainingHall") == 0)
        return trainingHall;

    if(name.compare("Library") == 0)
        return library;

    if(name.compare("Hatchery") == 0)
        return hatchery;

    if(name.compare("Crypt") == 0)
        return crypt;

    return nullRoomType;
}

int Room::costPerTile(RoomType t)
{
    switch (t)
    {
    case nullRoomType:
        return 0;

    case dungeonTemple:
        return 0;

    case portal:
        return 0;

    case treasury:
        return ConfigManager::getSingleton().getRoomConfigInt32("TreasuryCostPerTile");

    case dormitory:
        return ConfigManager::getSingleton().getRoomConfigInt32("DormitoryCostPerTile");

    case hatchery:
        return ConfigManager::getSingleton().getRoomConfigInt32("HatcheryCostPerTile");

    case forge:
        return ConfigManager::getSingleton().getRoomConfigInt32("ForgeCostPerTile");

    case trainingHall:
        return ConfigManager::getSingleton().getRoomConfigInt32("TrainHallCostPerTile");

    case library:
        return ConfigManager::getSingleton().getRoomConfigInt32("LibraryCostPerTile");

    case crypt:
        return ConfigManager::getSingleton().getRoomConfigInt32("CryptCostPerTile");

    default:
        return 0;
    }
}

void Room::setupRoom(const std::string& name, Seat* seat, const std::vector<Tile*>& tiles)
{
    setName(name);
    setSeat(seat);
    for(std::vector<Tile*>::const_iterator it = tiles.begin(); it != tiles.end(); ++it)
    {
        Tile* tile = *it;
        addCoveredTile(tile, Room::DEFAULT_TILE_HP);
    }
}

void Room::checkForRoomAbsorbtion()
{
    bool isRoomAbsorbed = false;
    for (Tile* tile : getGameMap()->tilesBorderedByRegion(getCoveredTiles()))
    {
        Room* room = tile->getCoveringRoom();
        if(room == nullptr)
            continue;
        if(room == this)
            continue;
        if(room->getSeat() != getSeat())
            continue;
        if(room->getType() != getType())
            continue;

        absorbRoom(room);
        // All the tiles from the absorbed room have been transfered to this one
        // No need to delete it since it will be removed during its next upkeep
        isRoomAbsorbed = true;
    }

    if(isRoomAbsorbed)
        reorderRoomAfterAbsorbtion();
}

void Room::updateActiveSpots()
{
    // Active spots are handled by the server only
    if(!getGameMap()->isServerGameMap())
        return;

    std::vector<Tile*> centralActiveSpotTiles;
    std::vector<Tile*> leftWallsActiveSpotTiles;
    std::vector<Tile*> rightWallsActiveSpotTiles;
    std::vector<Tile*> topWallsActiveSpotTiles;
    std::vector<Tile*> bottomWallsActiveSpotTiles;

    // Detect the centers of 3x3 squares tiles
    for(unsigned int i = 0, size = mCoveredTiles.size(); i < size; ++i)
    {
        bool foundTop = false;
        bool foundTopLeft = false;
        bool foundTopRight = false;
        bool foundLeft = false;
        bool foundRight = false;
        bool foundBottomLeft = false;
        bool foundBottomRight = false;
        bool foundBottom = false;
        Tile* tile = mCoveredTiles[i];
        int tileX = tile->getX();
        int tileY = tile->getY();

        // Check all other tiles to know whether we have one center tile.
        for(unsigned int j = 0; j < size; ++j)
        {
            // Don't check the tile itself
            if (tile == mCoveredTiles[j])
                continue;

            // Check whether the tile around the tile checked is already a center spot
            // as we can't have two center spots next to one another.
            if (std::find(centralActiveSpotTiles.begin(), centralActiveSpotTiles.end(), mCoveredTiles[j]) != centralActiveSpotTiles.end())
                continue;

            int tile2X = mCoveredTiles[j]->getX();
            int tile2Y = mCoveredTiles[j]->getY();

            if(tile2X == tileX - 1)
            {
                if (tile2Y == tileY + 1)
                    foundTopLeft = true;
                else if (tile2Y == tileY)
                    foundLeft = true;
                else if (tile2Y == tileY - 1)
                    foundBottomLeft = true;
            }
            else if(tile2X == tileX)
            {
                if (tile2Y == tileY + 1)
                    foundTop = true;
                else if (tile2Y == tileY - 1)
                    foundBottom = true;
            }
            else if(tile2X == tileX + 1)
            {
                if (tile2Y == tileY + 1)
                    foundTopRight = true;
                else if (tile2Y == tileY)
                    foundRight = true;
                else if (tile2Y == tileY - 1)
                    foundBottomRight = true;
            }
        }

        // Check whether we found a tile at the center of others
        if (foundTop && foundTopLeft && foundTopRight && foundLeft && foundRight
                && foundBottomLeft && foundBottomRight && foundBottom)
        {
            centralActiveSpotTiles.push_back(tile);
        }
    }

    // Now that we've got the center tiles, we can test the tile around for walls.
    for (unsigned int i = 0, size = centralActiveSpotTiles.size(); i < size; ++i)
    {
        Tile* centerTile = centralActiveSpotTiles[i];
        if (centerTile == nullptr)
            continue;

        // Test for walls around
        // Up
        Tile* testTile;
        testTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() + 2);
        if (testTile != nullptr && testTile->isWallClaimedForSeat(getSeat()))
        {
            Tile* topTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() + 1);
            if (topTile != nullptr)
                topWallsActiveSpotTiles.push_back(topTile);
        }
        // Up for 4 tiles wide room
        testTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() + 3);
        if (testTile != nullptr && testTile->isWallClaimedForSeat(getSeat()))
        {
            bool isFound = true;
            for(int k = 0; k < 3; ++k)
            {
                Tile* testTile2 = getGameMap()->getTile(centerTile->getX() + k - 1, centerTile->getY() + 2);
                if((testTile2 == nullptr) || (testTile2->getCoveringBuilding() != this))
                {
                    isFound = false;
                    break;
                }
            }

            if(isFound)
            {
                Tile* topTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() + 2);
                topWallsActiveSpotTiles.push_back(topTile);
            }
        }

        // Down
        testTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() - 2);
        if (testTile != nullptr && testTile->isWallClaimedForSeat(getSeat()))
        {
            Tile* bottomTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() - 1);
            if (bottomTile != nullptr)
                bottomWallsActiveSpotTiles.push_back(bottomTile);
        }
        // Down for 4 tiles wide room
        testTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() - 3);
        if (testTile != nullptr && testTile->isWallClaimedForSeat(getSeat()))
        {
            bool isFound = true;
            for(int k = 0; k < 3; ++k)
            {
                Tile* testTile2 = getGameMap()->getTile(centerTile->getX() + k - 1, centerTile->getY() - 2);
                if((testTile2 == nullptr) || (testTile2->getCoveringBuilding() != this))
                {
                    isFound = false;
                    break;
                }
            }

            if(isFound)
            {
                Tile* topTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() - 2);
                bottomWallsActiveSpotTiles.push_back(topTile);
            }
        }

        // Left
        testTile = getGameMap()->getTile(centerTile->getX() - 2, centerTile->getY());
        if (testTile != nullptr && testTile->isWallClaimedForSeat(getSeat()))
        {
            Tile* leftTile = getGameMap()->getTile(centerTile->getX() - 1, centerTile->getY());
            if (leftTile != nullptr)
                leftWallsActiveSpotTiles.push_back(leftTile);
        }
        // Left for 4 tiles wide room
        testTile = getGameMap()->getTile(centerTile->getX() - 3, centerTile->getY());
        if (testTile != nullptr && testTile->isWallClaimedForSeat(getSeat()))
        {
            bool isFound = true;
            for(int k = 0; k < 3; ++k)
            {
                Tile* testTile2 = getGameMap()->getTile(centerTile->getX() - 2, centerTile->getY() + k - 1);
                if((testTile2 == nullptr) || (testTile2->getCoveringBuilding() != this))
                {
                    isFound = false;
                    break;
                }
            }

            if(isFound)
            {
                Tile* topTile = getGameMap()->getTile(centerTile->getX() - 2, centerTile->getY());
                leftWallsActiveSpotTiles.push_back(topTile);
            }
        }

        // Right
        testTile = getGameMap()->getTile(centerTile->getX() + 2, centerTile->getY());
        if (testTile != nullptr && testTile->isWallClaimedForSeat(getSeat()))
        {
            Tile* rightTile = getGameMap()->getTile(centerTile->getX() + 1, centerTile->getY());
            if (rightTile != nullptr)
                rightWallsActiveSpotTiles.push_back(rightTile);
        }
        // Right for 4 tiles wide room
        testTile = getGameMap()->getTile(centerTile->getX() + 3, centerTile->getY());
        if (testTile != nullptr && testTile->isWallClaimedForSeat(getSeat()))
        {
            bool isFound = true;
            for(int k = 0; k < 3; ++k)
            {
                Tile* testTile2 = getGameMap()->getTile(centerTile->getX() + 2, centerTile->getY() + k - 1);
                if((testTile2 == nullptr) || (testTile2->getCoveringBuilding() != this))
                {
                    isFound = false;
                    break;
                }
            }

            if(isFound)
            {
                Tile* topTile = getGameMap()->getTile(centerTile->getX() + 2, centerTile->getY());
                rightWallsActiveSpotTiles.push_back(topTile);
            }
        }
    }

    activeSpotCheckChange(activeSpotCenter, mCentralActiveSpotTiles, centralActiveSpotTiles);
    activeSpotCheckChange(activeSpotLeft, mLeftWallsActiveSpotTiles, leftWallsActiveSpotTiles);
    activeSpotCheckChange(activeSpotRight, mRightWallsActiveSpotTiles, rightWallsActiveSpotTiles);
    activeSpotCheckChange(activeSpotTop, mTopWallsActiveSpotTiles, topWallsActiveSpotTiles);
    activeSpotCheckChange(activeSpotBottom, mBottomWallsActiveSpotTiles, bottomWallsActiveSpotTiles);

    mCentralActiveSpotTiles = centralActiveSpotTiles;
    mLeftWallsActiveSpotTiles = leftWallsActiveSpotTiles;
    mRightWallsActiveSpotTiles = rightWallsActiveSpotTiles;
    mTopWallsActiveSpotTiles = topWallsActiveSpotTiles;
    mBottomWallsActiveSpotTiles = bottomWallsActiveSpotTiles;

    // Updates the number of active spots
    mNumActiveSpots = mCentralActiveSpotTiles.size()
                      + mLeftWallsActiveSpotTiles.size() + mRightWallsActiveSpotTiles.size()
                      + mTopWallsActiveSpotTiles.size() + mBottomWallsActiveSpotTiles.size();
}

void Room::activeSpotCheckChange(ActiveSpotPlace place, const std::vector<Tile*>& originalSpotTiles,
    const std::vector<Tile*>& newSpotTiles)
{
    // We create the non existing tiles
    for(std::vector<Tile*>::const_iterator it = newSpotTiles.begin(); it != newSpotTiles.end(); ++it)
    {
        Tile* tile = *it;
        if(std::find(originalSpotTiles.begin(), originalSpotTiles.end(), tile) == originalSpotTiles.end())
        {
            // The tile do not exist
            RenderedMovableEntity* ro = notifyActiveSpotCreated(place, tile);
            if(ro != nullptr)
            {
                // The room wants to build a room onject. We add it to the gamemap
                addBuildingObject(tile, ro);
                ro->createMesh();
            }
        }
    }
    // We remove the suppressed tiles
    for(std::vector<Tile*>::const_iterator it = originalSpotTiles.begin(); it != originalSpotTiles.end(); ++it)
    {
        Tile* tile = *it;
        if(std::find(newSpotTiles.begin(), newSpotTiles.end(), tile) == newSpotTiles.end())
        {
            // The tile has been removed
            notifyActiveSpotRemoved(place, tile);
        }
    }
}

RenderedMovableEntity* Room::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    return nullptr;
}

void Room::notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
{
    removeBuildingObject(tile);
}

void Room::exportHeadersToStream(std::ostream& os)
{
    os << getType() << "\t";
}

void Room::exportHeadersToPacket(ODPacket& os)
{
    os << getType();
}

void Room::exportToPacket(ODPacket& os) const
{
    const std::string& name = getName();
    int seatId = getSeat()->getId();
    int nbTiles = mCoveredTiles.size();
    os << name << seatId << nbTiles;
    for (Tile* tile : mCoveredTiles)
    {
        os << tile->getX() << tile->getY();
    }
}

void Room::importFromPacket(ODPacket& is)
{
    std::string name;
    int tilesToLoad, tempX, tempY;
    OD_ASSERT_TRUE(is >> name);
    setName(name);
    int tempInt = 0;
    OD_ASSERT_TRUE(is >> tempInt);
    Seat* seat = getGameMap()->getSeatById(tempInt);
    OD_ASSERT_TRUE_MSG(seat != nullptr, "seatId=" + Ogre::StringConverter::toString(tempInt));
    setSeat(seat);

    OD_ASSERT_TRUE(is >> tilesToLoad);
    for (int i = 0; i < tilesToLoad; ++i)
    {
        OD_ASSERT_TRUE(is >> tempX >> tempY);
        Tile* tempTile = getGameMap()->getTile(tempX, tempY);
        OD_ASSERT_TRUE_MSG(tempTile != nullptr, "tile=" + Ogre::StringConverter::toString(tempX) + "," + Ogre::StringConverter::toString(tempY));
        if (tempTile != nullptr)
            addCoveredTile(tempTile, Room::DEFAULT_TILE_HP);
    }
}

void Room::exportToStream(std::ostream& os) const
{
    int seatId = getSeat()->getId();
    int nbTiles = mCoveredTiles.size();
    os << seatId << "\t" << nbTiles << "\n";
    for (Tile* tile : mCoveredTiles)
    {
        os << tile->getX() << "\t" << tile->getY() << "\n";
    }
}

void Room::importFromStream(std::istream& is)
{
    int tilesToLoad, tempX, tempY;
    int tempInt = 0;
    OD_ASSERT_TRUE(is >> tempInt);
    Seat* seat = getGameMap()->getSeatById(tempInt);
    OD_ASSERT_TRUE_MSG(seat != nullptr, "seatId=" + Ogre::StringConverter::toString(tempInt));
    setSeat(seat);

    OD_ASSERT_TRUE(is >> tilesToLoad);
    for (int i = 0; i < tilesToLoad; ++i)
    {
        OD_ASSERT_TRUE(is >> tempX >> tempY);
        Tile* tempTile = getGameMap()->getTile(tempX, tempY);
        OD_ASSERT_TRUE_MSG(tempTile != nullptr, "tile=" + Ogre::StringConverter::toString(tempX) + "," + Ogre::StringConverter::toString(tempY));
        if (tempTile != nullptr)
            addCoveredTile(tempTile, Room::DEFAULT_TILE_HP);
    }
}

bool Room::sortForMapSave(Room* r1, Room* r2)
{
    // We sort room by seat id then meshName
    int seatId1 = r1->getSeat()->getId();
    int seatId2 = r2->getSeat()->getId();
    if(seatId1 == seatId2)
        return r1->getMeshName().compare(r2->getMeshName()) < 0;

    return seatId1 < seatId2;
}

std::istream& operator>>(std::istream& is, Room::RoomType& rt)
{
    uint32_t tmp;
    is >> tmp;
    rt = static_cast<Room::RoomType>(tmp);
    return is;
}

std::ostream& operator<<(std::ostream& os, const Room::RoomType& rt)
{
    uint32_t tmp = static_cast<uint32_t>(rt);
    os << tmp;
    return os;
}

ODPacket& operator>>(ODPacket& is, Room::RoomType& rt)
{
    uint32_t tmp;
    is >> tmp;
    rt = static_cast<Room::RoomType>(tmp);
    return is;
}

ODPacket& operator<<(ODPacket& os, const Room::RoomType& rt)
{
    uint32_t tmp = static_cast<uint32_t>(rt);
    os << tmp;
    return os;
}
