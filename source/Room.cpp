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
#include "RoomObject.h"
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
    mType(nullRoomType),
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

    destroyRoomObjectMeshes();
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

Room* Room::createRoom(GameMap* gameMap, RoomType nType, const std::vector<Tile*>& nCoveredTiles,
    Seat* seat, bool forceName, const std::string& name)
{
    Room* tempRoom = NULL;

    switch (nType)
    {
    default:
    case nullRoomType:
        tempRoom = NULL;
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
    }

    if (tempRoom == NULL)
    {
        std::cerr << "ERROR: Trying to create a room of unknown type." << std::endl
                  << "Sourcefile: " << __FILE__ << "\tLine: " << __LINE__
                  << std::endl;
        return tempRoom;
    }

    tempRoom->setMeshExisting(false);
    tempRoom->setSeat(seat);

    //TODO: This should actually just call setType() but this will require a change to the >> operator.
    tempRoom->setMeshName(getMeshNameFromRoomType(nType));
    tempRoom->mType = nType;

    if(forceName)
        tempRoom->setName(name);
    else
        tempRoom->setName(gameMap->nextUniqueNameRoom(tempRoom->getMeshName()));

    for (unsigned int i = 0; i < nCoveredTiles.size(); ++i)
        tempRoom->addCoveredTile(nCoveredTiles[i]);

    int nbTiles = nCoveredTiles.size();
    LogManager::getSingleton().logMessage("Adding room " + tempRoom->getName() + ", nbTiles="
        + Ogre::StringConverter::toString(nbTiles) + ", seatId=" + Ogre::StringConverter::toString(seat->getId()));

    return tempRoom;
}

bool Room::compareTile(Tile* tile1, Tile* tile2)
{
    if(tile1->getX() < tile2->getX())
        return true;

    if(tile1->getX() == tile2->getX())
        return (tile1->getY() < tile2->getY());

    return false;
}

void Room::setupRoom(GameMap* gameMap, Room* newRoom)
{
    gameMap->addRoom(newRoom);
    std::vector<Tile*> coveredTiles = newRoom->getCoveredTiles();

    // Check all the tiles that border the newly created room and see if they
    // contain rooms which can be absorbed into this newly created room.
    bool isRoomAbsorbed = false;
    std::vector<Tile*> borderTiles = gameMap->tilesBorderedByRegion(coveredTiles);
    for (unsigned int i = 0; i < borderTiles.size(); ++i)
    {
        Room* borderingRoom = borderTiles[i]->getCoveringRoom();
        if (borderingRoom != NULL && borderingRoom->getType() == newRoom->getType()
            && borderingRoom != newRoom && borderingRoom->getSeat() == newRoom->getSeat())
        {
            newRoom->absorbRoom(borderingRoom);
            gameMap->removeRoom(borderingRoom);
            borderingRoom->deleteYourself();
            isRoomAbsorbed = true;
        }
    }

    // We try to keep the same tile disposition as if the room was created like this in the first
    // place to make sure room objects are disposed the same way
    if(isRoomAbsorbed)
        std::sort(newRoom->mCoveredTiles.begin(), newRoom->mCoveredTiles.end(), Room::compareTile);

    newRoom->createMesh();

    newRoom->updateActiveSpots();
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

    mRoomObjects.insert(r->mRoomObjects.begin(), r->mRoomObjects.end());
    r->mRoomObjects.clear();
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

void Room::addCoveredTile(Tile* t, double nHP, bool isRoomAbsorb)
{
    mCoveredTiles.push_back(t);
    mTileHP[t] = nHP;
    t->setCoveringRoom(this);
}

void Room::removeCoveredTile(Tile* t, bool isRoomAbsorb)
{
    bool removedTile = false;
    for (unsigned int i = 0; i < mCoveredTiles.size(); ++i)
    {
        if (t == mCoveredTiles[i])
        {
            mCoveredTiles.erase(mCoveredTiles.begin() + i);
            t->setCoveringRoom(NULL);
            mTileHP.erase(t);
            removedTile = true;
            break;
        }
    }

    if (!removedTile)
        return;

    if(getGameMap()->isServerGameMap())
        return;

    // Destroy the mesh for this tile.
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::destroyRoom;
    request->p = this;
    request->p2 = t;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);

    // NOTE: The active spot changes are done in upKeep()
}

Tile* Room::getCoveredTile(unsigned index)
{
    if (index >= mCoveredTiles.size())
        return NULL;

    return mCoveredTiles[index];
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

Tile* Room::getCentralTile()
{
    if (mCoveredTiles.empty())
        return NULL;

    int minX, maxX, minY, maxY;
    minX = maxX = mCoveredTiles[0]->getX();
    minY = maxY = mCoveredTiles[0]->getY();

    for(unsigned int i = 0, size = mCoveredTiles.size(); i < size; ++i)
    {
        int tempX = mCoveredTiles[i]->getX();
        int tempY = mCoveredTiles[i]->getY();

        if (tempX < minX)
            minX = tempX;
        if (tempY < minY)
            minY = tempY;
        if (tempX > maxX)
            maxX = tempX;
        if (tempY > maxY)
            maxY = tempY;
    }

    return getGameMap()->getTile((minX + maxX) / 2, (minY + maxY) / 2);
}

RoomObject* Room::loadRoomObject(GameMap* gameMap, const std::string& meshName,
    Tile* targetTile, double rotationAngle)
{
    if (targetTile == NULL)
        targetTile = getCentralTile();

    return loadRoomObject(gameMap, meshName, targetTile, static_cast<double>(targetTile->x),
        static_cast<double>(targetTile->y), rotationAngle);
}

RoomObject* Room::loadRoomObject(GameMap* gameMap, const std::string& meshName,
    Tile* targetTile, double x, double y, double rotationAngle)
{
    RoomObject* tempRoomObject = new RoomObject(gameMap, this, meshName);
    tempRoomObject->mX = (Ogre::Real)x;
    tempRoomObject->mY = (Ogre::Real)y;
    tempRoomObject->mRotationAngle = (Ogre::Real)rotationAngle;

    return tempRoomObject;
}

void Room::addRoomObject(Tile* targetTile, RoomObject* roomObject)
{
    if(roomObject == NULL)
        return;

    if(getGameMap()->isServerGameMap())
    {
        LogManager::getSingleton().logMessage("Adding room object " + roomObject->getName()
            + ",room " + getName() + ",MeshName=" + roomObject->getMeshName()
            + ",tile=" + Tile::displayAsString(targetTile));
        try
        {
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotification::addRoomObject, NULL);
            std::string name = getName();
            serverNotification->mPacket << name << targetTile << roomObject;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
        catch (std::bad_alloc&)
        {
            Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in Room::addRoomObject", Ogre::LML_CRITICAL);
            exit(1);
        }
    }
    Ogre::Vector3 objPos(static_cast<Ogre::Real>(targetTile->x), static_cast<Ogre::Real>(targetTile->y), 0);
    roomObject->setPosition(objPos);
    mRoomObjects[targetTile] = roomObject;
    getGameMap()->addAnimatedObject(roomObject);
}

void Room::removeRoomObject(Tile* tile)
{
    if(mRoomObjects.count(tile) == 0)
        return;

    RoomObject* roomObject = mRoomObjects[tile];
    if(getGameMap()->isServerGameMap())
    {
        LogManager::getSingleton().logMessage("Removing room object " + roomObject->getName()
            + " in room " + getName());
        try
        {
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotification::removeRoomObject, NULL);
            std::string name = getName();
            serverNotification->mPacket << name << tile;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
        catch (std::bad_alloc&)
        {
            Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in Room::removeRoomObject", Ogre::LML_CRITICAL);
            exit(1);
        }
    }
    getGameMap()->removeAnimatedObject(roomObject);
    roomObject->deleteYourself();
    mRoomObjects.erase(tile);
}

void Room::removeRoomObject(RoomObject* roomObject)
{
    std::map<Tile*, RoomObject*>::iterator it;

    for (it = mRoomObjects.begin(); it != mRoomObjects.end(); ++it)
    {
        if(it->second == roomObject)
            break;
    }

    if(it != mRoomObjects.end())
    {
        if(getGameMap()->isServerGameMap())
        {
            LogManager::getSingleton().logMessage("Removing room object " + roomObject->getName()
                + " in room " + getName());
            try
            {
                Tile* tile = it->first;
                ServerNotification *serverNotification = new ServerNotification(
                    ServerNotification::removeRoomObject, NULL);
                std::string name = getName();
                serverNotification->mPacket << name << tile;
                ODServer::getSingleton().queueServerNotification(serverNotification);
            }
            catch (std::bad_alloc&)
            {
                Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in Room::removeRoomObject", Ogre::LML_CRITICAL);
                exit(1);
            }
        }
        getGameMap()->removeAnimatedObject(roomObject);
        roomObject->deleteYourself();
        mRoomObjects.erase(it);
    }
}

void Room::removeAllRoomObject()
{
    if(mRoomObjects.size() == 0)
        return;

    if(getGameMap()->isServerGameMap())
    {
        try
        {
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotification::removeAllRoomObjectFromRoom, NULL);
            std::string name = getName();
            serverNotification->mPacket << name;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
        catch (std::bad_alloc&)
        {
            Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in Room::removeAllRoomObject", Ogre::LML_CRITICAL);
            exit(1);
        }
    }
    std::map<Tile*, RoomObject*>::iterator itr = mRoomObjects.begin();
    while (itr != mRoomObjects.end())
    {
        RoomObject* roomObject = itr->second;
        getGameMap()->removeAnimatedObject(roomObject);
        roomObject->deleteYourself();
        ++itr;
    }
    mRoomObjects.clear();
}

RoomObject* Room::getRoomObjectFromTile(Tile* tile)
{
    if(mRoomObjects.count(tile) == 0)
        return NULL;

    RoomObject* tempRoomObject = mRoomObjects[tile];
    return tempRoomObject;
}

RoomObject* Room::getFirstRoomObject()
{
    if (!mRoomObjects.empty())
        return mRoomObjects.begin()->second;
    else
        return NULL;
}

void Room::createRoomObjectMeshes()
{
    // Loop over all the RoomObjects that are children of this room and create each mesh individually.
    for(std::map<Tile*, RoomObject*>::iterator it = mRoomObjects.begin(); it != mRoomObjects.end(); ++it)
    {
        RoomObject* ro = it->second;
        ro->createMesh();
    }
}

void Room::destroyRoomObjectMeshes()
{
    // Loop over all the RoomObjects that are children of this room and destroy each mesh individually.
    std::map<Tile*, RoomObject*>::iterator itr = mRoomObjects.begin();
    while (itr != mRoomObjects.end())
    {
        itr->second->destroyMesh();
        ++itr;
    }
}

std::string Room::getFormat()
{
    return "meshName\tseatId\t\tNextLine: numTiles\t\tSubsequent Lines: tileX\ttileY";
}

bool Room::doUpkeep()
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
        getGameMap()->removeRoom(this);
        deleteYourself();
    }

    return true;
}

Room* Room::createRoomFromStream(GameMap* gameMap, const std::string& roomMeshName, std::istream& is,
    const std::string& roomName)
{
    Room tempRoom(gameMap);
    tempRoom.setMeshName(roomMeshName);
    is >> &tempRoom;

    return createRoom(gameMap, tempRoom.mType, tempRoom.mCoveredTiles, tempRoom.getSeat(),
        !roomName.empty(), roomName);
}

std::istream& operator>>(std::istream& is, Room* r)
{
    assert(r);

    int tilesToLoad, tempX, tempY;
    int tempInt = 0;
    is >> tempInt;
    Seat* seat = r->getGameMap()->getSeatById(tempInt);
    OD_ASSERT_TRUE_MSG(seat != NULL, "seatId=" + Ogre::StringConverter::toString(tempInt));
    r->setSeat(seat);

    is >> tilesToLoad;
    for (int i = 0; i < tilesToLoad; ++i)
    {
        is >> tempX >> tempY;
        Tile* tempTile = r->getGameMap()->getTile(tempX, tempY);
        if (tempTile != NULL)
            r->addCoveredTile(tempTile);
    }

    r->mType = Room::getRoomTypeFromMeshName(r->getMeshName());
    return is;
}

std::ostream& operator<<(std::ostream& os, Room *r)
{
    if (r == NULL)
        return os;

    int seatId = r->getSeat()->getId();
    os << r->getMeshName() << "\t" << seatId << "\n";
    os << r->mCoveredTiles.size() << "\n";
    for (unsigned int i = 0; i < r->mCoveredTiles.size(); ++i)
    {
        Tile *tempTile = r->mCoveredTiles[i];
        os << tempTile->x << "\t" << tempTile->y << "\n";
    }

    return os;
}

Room* Room::createRoomFromPacket(GameMap* gameMap, const std::string& roomMeshName, ODPacket& is,
    const std::string& roomName)
{
    Room tempRoom(gameMap);
    tempRoom.setMeshName(roomMeshName);
    is >> &tempRoom;

    return createRoom(gameMap, tempRoom.mType, tempRoom.mCoveredTiles, tempRoom.getSeat(),
        !roomName.empty(), roomName);
}

ODPacket& operator>>(ODPacket& is, Room* r)
{
    assert(r);

    int tilesToLoad, tempX, tempY;

    int tempInt = 0;
    is >> tempInt;
    Seat* seat = r->getGameMap()->getSeatById(tempInt);
    OD_ASSERT_TRUE_MSG(seat != NULL, "seatId=" + Ogre::StringConverter::toString(tempInt));
    r->setSeat(seat);

    is >> tilesToLoad;
    for (int i = 0; i < tilesToLoad; ++i)
    {
        is >> tempX >> tempY;
        Tile* tempTile = r->getGameMap()->getTile(tempX, tempY);
        if (tempTile != NULL)
            r->addCoveredTile(tempTile);
    }

    r->mType = Room::getRoomTypeFromMeshName(r->getMeshName());
    return is;
}

ODPacket& operator<<(ODPacket& os, Room *r)
{
    if (r == NULL)
        return os;

    std::string meshName = r->getMeshName();
    std::string name = r->getName();
    int seatId = r->getSeat()->getId();
    int nbTiles = r->mCoveredTiles.size();
    os << meshName << name << seatId << nbTiles;
    for (std::vector<Tile*>::iterator it = r->mCoveredTiles.begin(); it != r->mCoveredTiles.end(); ++it)
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

const char* Room::getMeshNameFromRoomType(RoomType t)
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
        return "Dojo";

    case library:
        return "Library";

    case hatchery:
        return "Farm";

    default:
        return "UnknownRoomType";
    }
}

Room::RoomType Room::getRoomTypeFromMeshName(const std::string& s)
{
    if (s.compare("DungeonTemple") == 0)
        return dungeonTemple;
    else if (s.compare("Dormitory") == 0)
        return dormitory;
    else if (s.compare("Treasury") == 0)
        return treasury;
    else if (s.compare("Portal") == 0)
        return portal;
    else if (s.compare("Forge") == 0)
        return forge;
    else if (s.compare("Dojo") == 0)
        return trainingHall;
    else if (s.compare("Library") == 0)
        return library;
    else if (s.compare("Farm") == 0)
        return hatchery;
    else
    {
        std::cerr << "\n\n\nERROR:  Trying to get room type from unknown mesh name, bailing out.\n";
        std::cerr << "Sourcefile: " << __FILE__ << "\tLine: " << __LINE__ << "\n\n\n";
        return nullRoomType;
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

double Room::getHP(Tile *tile)
{
    //NOTE: This function is the same as Trap::getHP(), consider making a base class to inherit this from.
    if (tile != NULL)
        return mTileHP[tile];

    // If the tile give was NULL, we add the total HP of all the tiles in the room and return that.
    double total = 0.0;

    for(std::map<Tile*, double>::iterator itr = mTileHP.begin(), end = mTileHP.end();
        itr != end; ++itr)
    {
        total += itr->second;
    }

    return total;
}

void Room::takeDamage(GameEntity* attacker, double damage, Tile* tileTakingDamage)
{
    if (tileTakingDamage == NULL)
        return;

    mTileHP[tileTakingDamage] -= damage;

    GameMap* gameMap = getGameMap();
    if (gameMap == NULL)
        return;

    if(!gameMap->isServerGameMap())
        return;

    Seat* seat = getSeat();
    if (seat == NULL)
        return;

    Player* player = gameMap->getPlayerBySeatId(seat->getId());
    if (player == NULL)
        return;

    // Tells the server game map the player is under attack.
    gameMap->playerIsFighting(player);
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
        // Up for 4 tiles
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
        // Down for 4 tiles
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
        // Left for 4 tiles
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
        // Right for 4 tiles
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
            RoomObject* ro = notifyActiveSpotCreated(place, tile);
            if(ro != NULL)
            {
                // The room wants to build a room onject. We add it to the gamemap
                addRoomObject(tile, ro);
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

RoomObject* Room::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    return NULL;
}

void Room::notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
{
    removeRoomObject(tile);
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
