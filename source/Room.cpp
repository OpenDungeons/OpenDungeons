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

#include "Room.h"

#include "RoomTrainingHall.h"
#include "RoomDungeonTemple.h"
#include "RoomForge.h"
#include "RoomLibrary.h"
#include "RoomPortal.h"
#include "RoomDormitory.h"
#include "RoomTreasury.h"
#include "RoomHatchery.h"
#include "RenderedMovableEntity.h"
#include "ODServer.h"
#include "ServerNotification.h"
#include "Player.h"
#include "Tile.h"
#include "Creature.h"
#include "RenderRequest.h"
#include "GameMap.h"
#include "RenderManager.h"
#include "Seat.h"
#include "LogManager.h"

#include <sstream>

Room::Room(GameMap* gameMap):
    Building(gameMap),
    mNumActiveSpots(0)
{
    setObjectType(GameEntity::room);
}

void Room::createMeshLocal()
{
    Building::createMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    std::vector<Tile*> coveredTiles = getCoveredTiles();
    for (unsigned int i = 0, nb = coveredTiles.size(); i < nb; ++i)
    {
        RenderRequest* request = new RenderRequest;
        request->type = RenderRequest::createRoom;
        request->p    = static_cast<void*>(this);
        request->p2   = coveredTiles[i];
        RenderManager::queueRenderRequest(request);
    }
}

void Room::destroyMeshLocal()
{
    Building::destroyMeshLocal();

    destroyBuildingObjectMeshes();
    if(getGameMap()->isServerGameMap())
        return;

    std::vector<Tile*> coveredTiles = getCoveredTiles();
    for (unsigned int i = 0, nb = coveredTiles.size(); i < nb; ++i)
    {
        RenderRequest* request = new RenderRequest;
        request->type = RenderRequest::destroyRoom;
        request->p    = static_cast<void*>(this);
        request->p2   = coveredTiles[i];
        RenderManager::queueRenderRequest(request);
    }
}

void Room::deleteYourselfLocal()
{
    Building::deleteYourselfLocal();
    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::deleteRoom;
    request->p = static_cast<void*>(this);
    RenderManager::queueRenderRequest(request);
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

    mBuildingObjects.insert(r->mBuildingObjects.begin(), r->mBuildingObjects.end());
    r->mBuildingObjects.clear();
    // Every creature working in this room should go to the new one (this is used
    // in the server map only)
    if(getGameMap()->isServerGameMap())
    {
        while(r->mCreaturesUsingRoom.size() > 0)
        {
            Creature* creature = r->mCreaturesUsingRoom[0];
            if(creature->isJobRoom(r))
                creature->changeJobRoom(this);
            else if(creature->isEatRoom(r))
                creature->changeEatRoom(this);
            else
            {
                OD_ASSERT_TRUE(false);
                r->mCreaturesUsingRoom.erase(r->mCreaturesUsingRoom.begin());
            }
        }
    }

    while (r->numCoveredTiles() > 0)
    {
        Tile *tempTile = r->getCoveredTile(0);
        double hp = r->getHP(tempTile);
        r->removeCoveredTile(tempTile, true);
        addCoveredTile(tempTile, hp, true);
    }
}

void Room::addCoveredTile(Tile* t, double nHP)
{
    addCoveredTile(t, nHP, false);
}

void Room::addCoveredTile(Tile* t, double nHP, bool isRoomAbsorb)
{
    Building::addCoveredTile(t, nHP);
    t->setCoveringRoom(this);
}

bool Room::removeCoveredTile(Tile* t)
{
    return removeCoveredTile(t, false);
}

bool Room::removeCoveredTile(Tile* t, bool isRoomAbsorb)
{
    if(!Building::removeCoveredTile(t))
        return false;

    t->setCoveringRoom(NULL);

    if(getGameMap()->isServerGameMap())
        return true;

    // Destroy the mesh for this tile.
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::destroyRoom;
    request->p = this;
    request->p2 = t;
    RenderManager::queueRenderRequest(request);

    // NOTE: The active spot changes are done in upKeep()
    return true;
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
        return NULL;

    return mCreaturesUsingRoom[index];
}

void Room::createBuildingObjectMeshes()
{
    // Loop over all the RenderedMovableEntity that are children of this room and create each mesh individually.
    for(std::map<Tile*, RenderedMovableEntity*>::iterator it = mBuildingObjects.begin(); it != mBuildingObjects.end(); ++it)
    {
        RenderedMovableEntity* ro = it->second;
        ro->createMesh();
    }
}

void Room::destroyBuildingObjectMeshes()
{
    // Loop over all the BuildingObjects that are children of this room and destroy each mesh individually.
    std::map<Tile*, RenderedMovableEntity*>::iterator itr = mBuildingObjects.begin();
    while (itr != mBuildingObjects.end())
    {
        itr->second->destroyMesh();
        ++itr;
    }
}

std::string Room::getFormat()
{
    return "typeRoom\tseatId\tnumTiles\t\tSubsequent Lines: tileX\ttileY";
}

void Room::doUpkeep()
{
    // Loop over the tiles in Room r and remove any whose HP has dropped to zero.
    unsigned int i = 0;
    bool oneTileRemoved = false;
    while (i < mCoveredTiles.size())
    {
        Tile* t = mCoveredTiles[i];
        if (mTileHP[t] <= 0.0)
        {
            if(getGameMap()->isServerGameMap())
            {
                try
                {
                    ServerNotification *serverNotification = new ServerNotification(
                        ServerNotification::removeRoomTile, NULL);
                    std::string name = getName();
                    serverNotification->mPacket << name << t;
                    ODServer::getSingleton().queueServerNotification(serverNotification);
                }
                catch (std::bad_alloc&)
                {
                    Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in Room::removeCoveredTile", Ogre::LML_CRITICAL);
                    exit(1);
                }
            }
            removeCoveredTile(t);
            oneTileRemoved = true;
        }
        else
            ++i;
    }

    if (oneTileRemoved)
    {
        updateActiveSpots();
        createMesh();
    }

    // If no more tiles, the room is removed
    if (numCoveredTiles() == 0)
    {
        LogManager::getSingleton().logMessage("Removing room " + getName());
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
            is >> tempRoom;
            break;
        case treasury:
            tempRoom = new RoomTreasury(gameMap);
            is >> tempRoom;
            break;
        case portal:
            tempRoom = new RoomPortal(gameMap);
            is >> tempRoom;
            break;
        case dungeonTemple:
            tempRoom = new RoomDungeonTemple(gameMap);
            is >> tempRoom;
            break;
        case forge:
            tempRoom = new RoomForge(gameMap);
            is >> tempRoom;
            break;
        case trainingHall:
            tempRoom = new RoomTrainingHall(gameMap);
            is >> tempRoom;
            break;
        case library:
            tempRoom = new RoomLibrary(gameMap);
            is >> tempRoom;
            break;
        case hatchery:
            tempRoom = new RoomHatchery(gameMap);
            is >> tempRoom;
            break;
        default:
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(nType)));
    }

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
            is >> tempRoom;
            break;
        case treasury:
            tempRoom = new RoomTreasury(gameMap);
            is >> tempRoom;
            break;
        case portal:
            tempRoom = new RoomPortal(gameMap);
            is >> tempRoom;
            break;
        case dungeonTemple:
            tempRoom = new RoomDungeonTemple(gameMap);
            is >> tempRoom;
            break;
        case forge:
            tempRoom = new RoomForge(gameMap);
            is >> tempRoom;
            break;
        case trainingHall:
            tempRoom = new RoomTrainingHall(gameMap);
            is >> tempRoom;
            break;
        case library:
            tempRoom = new RoomLibrary(gameMap);
            is >> tempRoom;
            break;
        case hatchery:
            tempRoom = new RoomHatchery(gameMap);
            is >> tempRoom;
            break;
        default:
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(nType)));
    }

    return tempRoom;
}

void Room::exportToPacket(ODPacket& packet)
{
    packet << this;
}

void Room::exportToStream(std::ostream& os)
{
    os << this;
}

std::istream& operator>>(std::istream& is, Room* room)
{
    int tilesToLoad, tempX, tempY;
    int tempInt = 0;
    is >> tempInt;
    Seat* seat = room->getGameMap()->getSeatById(tempInt);
    OD_ASSERT_TRUE_MSG(seat != NULL, "seatId=" + Ogre::StringConverter::toString(tempInt));
    room->setSeat(seat);

    is >> tilesToLoad;
    for (int i = 0; i < tilesToLoad; ++i)
    {
        is >> tempX >> tempY;
        Tile* tempTile = room->getGameMap()->getTile(tempX, tempY);
        if (tempTile != NULL)
            room->addCoveredTile(tempTile, Room::DEFAULT_TILE_HP, false);
    }

    return is;
}

std::ostream& operator<<(std::ostream& os, Room *room)
{
    int seatId = room->getSeat()->getId();
    int nbTiles = room->mCoveredTiles.size();
    os << seatId << "\t" << nbTiles << "\n";
    for (std::vector<Tile*>::iterator it = room->mCoveredTiles.begin(); it != room->mCoveredTiles.end(); ++it)
    {
        Tile *tempTile = *it;
        os << tempTile->x << "\t" << tempTile->y << "\n";
    }

    return os;
}

ODPacket& operator>>(ODPacket& is, Room* room)
{
    std::string name;
    int tilesToLoad, tempX, tempY;
    is >> name;
    room->setName(name);
    int tempInt = 0;
    is >> tempInt;
    Seat* seat = room->getGameMap()->getSeatById(tempInt);
    OD_ASSERT_TRUE_MSG(seat != NULL, "seatId=" + Ogre::StringConverter::toString(tempInt));
    room->setSeat(seat);

    is >> tilesToLoad;
    for (int i = 0; i < tilesToLoad; ++i)
    {
        is >> tempX >> tempY;
        Tile* tempTile = room->getGameMap()->getTile(tempX, tempY);
        if (tempTile != NULL)
            room->addCoveredTile(tempTile, Room::DEFAULT_TILE_HP, false);
    }

    return is;
}

ODPacket& operator<<(ODPacket& os, Room *room)
{
    const std::string& name = room->getName();
    int seatId = room->getSeat()->getId();
    int nbTiles = room->mCoveredTiles.size();
    os << name << seatId << nbTiles;
    for (std::vector<Tile*>::iterator it = room->mCoveredTiles.begin(); it != room->mCoveredTiles.end(); ++it)
    {
        Tile* tempTile = *it;
        os << tempTile->x << tempTile->y;
    }

    return os;
}

const char* Room::getRoomNameFromRoomType(RoomType t)
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

    default:
        return "UnknownRoomType";
    }
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

    case dormitory:
        return 75;

    case treasury:
        return 25;

    case forge:
        return 150;

    case trainingHall:
        return 175;

    case library:
        return 200;

    case hatchery:
        return 100;

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
    std::vector<Tile*> borderTiles = getGameMap()->tilesBorderedByRegion(getCoveredTiles());
    for (std::vector<Tile*>::iterator it = borderTiles.begin(); it != borderTiles.end(); ++it)
    {
        Tile* tile = *it;
        Room* borderingRoom = tile->getCoveringRoom();
        if (borderingRoom != nullptr && borderingRoom->getType() == getType()
            && borderingRoom != this && borderingRoom->getSeat() == getSeat())
        {
            absorbRoom(borderingRoom);
            // All the tiles from the absorbed room have been transfered to this one
            // No need to delete it since it will be removed during its next upkeep
            isRoomAbsorbed = true;
        }
    }

    // We try to keep the same tile disposition as if the room was created like this in the first
    // place to make sure building objects are disposed the same way
    if(isRoomAbsorbed)
        std::sort(mCoveredTiles.begin(), mCoveredTiles.end(), Room::compareTile);
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
        if (centerTile == NULL)
            continue;

        // Test for walls around
        // Up
        Tile* testTile;
        testTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() + 2);
        if (testTile != NULL && testTile->isWallClaimedForSeat(getSeat()))
        {
            Tile* topTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() + 1);
            if (topTile != NULL)
                topWallsActiveSpotTiles.push_back(topTile);
        }
        // Up for 4 tiles wide room
        testTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() + 3);
        if (testTile != NULL && testTile->isWallClaimedForSeat(getSeat()))
        {
            bool isFound = true;
            for(int k = 0; k < 3; ++k)
            {
                Tile* testTile2 = getGameMap()->getTile(centerTile->getX() + k - 1, centerTile->getY() + 2);
                if((testTile2 == NULL) || (testTile2->getCoveringRoom() != this))
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
        if (testTile != NULL && testTile->isWallClaimedForSeat(getSeat()))
        {
            Tile* bottomTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() - 1);
            if (bottomTile != NULL)
                bottomWallsActiveSpotTiles.push_back(bottomTile);
        }
        // Down for 4 tiles wide room
        testTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() - 3);
        if (testTile != NULL && testTile->isWallClaimedForSeat(getSeat()))
        {
            bool isFound = true;
            for(int k = 0; k < 3; ++k)
            {
                Tile* testTile2 = getGameMap()->getTile(centerTile->getX() + k - 1, centerTile->getY() - 2);
                if((testTile2 == NULL) || (testTile2->getCoveringRoom() != this))
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
        if (testTile != NULL && testTile->isWallClaimedForSeat(getSeat()))
        {
            Tile* leftTile = getGameMap()->getTile(centerTile->getX() - 1, centerTile->getY());
            if (leftTile != NULL)
                leftWallsActiveSpotTiles.push_back(leftTile);
        }
        // Left for 4 tiles wide room
        testTile = getGameMap()->getTile(centerTile->getX() - 3, centerTile->getY());
        if (testTile != NULL && testTile->isWallClaimedForSeat(getSeat()))
        {
            bool isFound = true;
            for(int k = 0; k < 3; ++k)
            {
                Tile* testTile2 = getGameMap()->getTile(centerTile->getX() - 2, centerTile->getY() + k - 1);
                if((testTile2 == NULL) || (testTile2->getCoveringRoom() != this))
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
        if (testTile != NULL && testTile->isWallClaimedForSeat(getSeat()))
        {
            Tile* rightTile = getGameMap()->getTile(centerTile->getX() + 1, centerTile->getY());
            if (rightTile != NULL)
                rightWallsActiveSpotTiles.push_back(rightTile);
        }
        // Right for 4 tiles wide room
        testTile = getGameMap()->getTile(centerTile->getX() + 3, centerTile->getY());
        if (testTile != NULL && testTile->isWallClaimedForSeat(getSeat()))
        {
            bool isFound = true;
            for(int k = 0; k < 3; ++k)
            {
                Tile* testTile2 = getGameMap()->getTile(centerTile->getX() + 2, centerTile->getY() + k - 1);
                if((testTile2 == NULL) || (testTile2->getCoveringRoom() != this))
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
            if(ro != NULL)
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
    return NULL;
}

void Room::notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
{
    removeBuildingObject(tile);
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
