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

#include "Trap.h"

#include "ODServer.h"
#include "ServerNotification.h"
#include "Tile.h"
#include "RenderRequest.h"
#include "RenderManager.h"
#include "Seat.h"
#include "GameMap.h"
#include "RoomObject.h"
#include "TrapCannon.h"
#include "TrapSpike.h"
#include "TrapBoulder.h"
#include "Random.h"
#include "Player.h"
#include "LogManager.h"

const double Trap::mDefaultTileHP = 10.0;

Trap::Trap(GameMap* gameMap) :
    Building(gameMap),
    mReloadTime(0),
    mMinDamage(0.0),
    mMaxDamage(0.0),
    mType(nullTrapType)
{
    setObjectType(GameEntity::trap);
}

void Trap::createMeshLocal()
{
    Building::createMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    std::vector<Tile*> coveredTiles = getCoveredTiles();
    for (unsigned int i = 0, nb = coveredTiles.size(); i < nb; ++i)
    {
        RenderRequest* request = new RenderRequest;
        request->type = RenderRequest::createTrap;
        request->p    = static_cast<void*>(this);
        request->p2   = coveredTiles[i];
        RenderManager::queueRenderRequest(request);
    }
}

void Trap::destroyMeshLocal()
{
    Building::destroyMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    std::vector<Tile*> coveredTiles = getCoveredTiles();
    for (unsigned int i = 0, nb = coveredTiles.size(); i < nb; ++i)
    {
        RenderRequest *request = new RenderRequest;
        request->type = RenderRequest::destroyTrap;
        request->p    = static_cast<void*>(this);
        request->p2   = coveredTiles[i];
        RenderManager::queueRenderRequest(request);
    }
}

void Trap::deleteYourselfLocal()
{
    Building::deleteYourselfLocal();
    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::deleteTrap;
    request->p = static_cast<void*>(this);
    RenderManager::queueRenderRequest(request);
}

Trap* Trap::createTrap(GameMap* gameMap, TrapType nType, const std::vector<Tile*> &nCoveredTiles,
    Seat *seat, bool forceName, const std::string& name, void* params)
{
    Trap *tempTrap = NULL;

    switch (nType)
    {
        case nullTrapType:
            tempTrap = NULL;
            break;
        case cannon:
            tempTrap = new TrapCannon(gameMap);
            break;
        case spike:
            tempTrap = new TrapSpike(gameMap);
            break;
        case boulder:
            if(params != NULL)
            {
                int* p = (int*)params;
                tempTrap = new TrapBoulder(gameMap, p[0], p[1]);
            }
            break;
    }

    if (tempTrap == NULL)
    {
        std::cerr
        << "\n\n\nERROR: Trying to create a trap of unknown type, bailing out.\n";
        std::cerr << "Sourcefile: " << __FILE__ << "\tLine: " << __LINE__
        << "\n\n\n";
        exit(1);
    }

    tempTrap->setSeat(seat);

    tempTrap->setMeshName(getMeshNameFromTrapType(nType));
    tempTrap->mType = nType;

    if(forceName)
        tempTrap->setName(name);
    else
        tempTrap->setName(gameMap->nextUniqueNameTrap(tempTrap->getMeshName()));

    for (unsigned int i = 0; i < nCoveredTiles.size(); ++i)
        tempTrap->addCoveredTile(nCoveredTiles[i]);

    int nbTiles = nCoveredTiles.size();
    LogManager::getSingleton().logMessage("Adding trap " + tempTrap->getName() + ", nbTiles="
        + Ogre::StringConverter::toString(nbTiles) + ", seatId=" + Ogre::StringConverter::toString(seat->getId()));

    return tempTrap;
}

void Trap::setupTrap(GameMap* gameMap, Trap* newTrap)
{
    gameMap->addTrap(newTrap);

    newTrap->createMesh();

    newTrap->updateActiveSpots();
}

void Trap::addRoomObject(Tile* targetTile, RoomObject* roomObject)
{
    if(roomObject == NULL)
        return;

    LogManager::getSingleton().logMessage("Adding game object " + roomObject->getName()
        + ",trap=" + getName() + ",MeshName=" + roomObject->getMeshName()
        + ",tile=" + Tile::displayAsString(targetTile));

    Ogre::Vector3 objPos(static_cast<Ogre::Real>(targetTile->x), static_cast<Ogre::Real>(targetTile->y), 0);
    roomObject->setPosition(objPos);
    mRoomObjects[targetTile] = roomObject;
    getGameMap()->addRoomObject(roomObject);
}

void Trap::removeRoomObject(Tile* tile)
{
    if(mRoomObjects.count(tile) == 0)
        return;

    RoomObject* roomObject = mRoomObjects[tile];
    LogManager::getSingleton().logMessage("Removing trap object " + roomObject->getName()
        + " in trap=" + getName());
    getGameMap()->removeRoomObject(roomObject);
    roomObject->deleteYourself();
    mRoomObjects.erase(tile);
}

void Trap::removeRoomObject(RoomObject* roomObject)
{
    std::map<Tile*, RoomObject*>::iterator it;

    for (it = mRoomObjects.begin(); it != mRoomObjects.end(); ++it)
    {
        if(it->second == roomObject)
            break;
    }

    if(it != mRoomObjects.end())
    {
        LogManager::getSingleton().logMessage("Removing trap object " + roomObject->getName()
            + " in room " + getName());
        getGameMap()->removeRoomObject(roomObject);
        roomObject->deleteYourself();
        mRoomObjects.erase(it);
    }
}

void Trap::removeAllRoomObject()
{
    if(mRoomObjects.empty())
        return;

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

RoomObject* Trap::getRoomObjectFromTile(Tile* tile)
{
    if(mRoomObjects.count(tile) == 0)
        return NULL;

    RoomObject* tempRoomObject = mRoomObjects[tile];
    return tempRoomObject;
}

RoomObject* Trap::loadRoomObject(GameMap* gameMap, const std::string& meshName,
    Tile* targetTile, double rotationAngle)
{
    OD_ASSERT_TRUE(targetTile != NULL);
    if(targetTile == NULL)
        return NULL;

    return loadRoomObject(gameMap, meshName, targetTile, static_cast<double>(targetTile->x),
        static_cast<double>(targetTile->y), rotationAngle);
}

RoomObject* Trap::loadRoomObject(GameMap* gameMap, const std::string& meshName,
    Tile* targetTile, double x, double y, double rotationAngle)
{
    RoomObject* tempRoomObject = new RoomObject(gameMap, getName(), meshName);
    tempRoomObject->mX = (Ogre::Real)x;
    tempRoomObject->mY = (Ogre::Real)y;
    tempRoomObject->mRotationAngle = (Ogre::Real)rotationAngle;

    return tempRoomObject;
}

 Trap* Trap::createTrapFromStream(GameMap* gameMap, const std::string& trapMeshName, std::istream &is,
    const std::string& trapName)
{
    Trap tempTrap(gameMap);
    tempTrap.setMeshName(trapMeshName);
    is >> &tempTrap;

    Trap *returnTrap = createTrap(gameMap, tempTrap.mType, tempTrap.mCoveredTiles,
        tempTrap.getSeat(), !trapName.empty(), trapName);
    return returnTrap;
}

Trap* Trap::createTrapFromPacket(GameMap* gameMap, const std::string& trapMeshName, ODPacket &is,
    const std::string& trapName)
{
    Trap tempTrap(gameMap);
    tempTrap.setMeshName(trapMeshName);
    is >> &tempTrap;

    Trap *returnTrap = createTrap(gameMap, tempTrap.mType, tempTrap.mCoveredTiles,
        tempTrap.getSeat(), !trapName.empty(), trapName);
    return returnTrap;
}

const char* Trap::getTrapNameFromTrapType(TrapType t)
{
    switch (t)
    {
        case nullTrapType:
            return "NullTrapType";

        case cannon:
            return "Cannon";

        case spike:
            return "Spike";

        case boulder:
            return "Boulder";

        default:
            return "UnknownTrapType";
    }
}

const char* Trap::getMeshNameFromTrapType(TrapType t)
{
    switch (t)
    {
        case nullTrapType:
            return "NullTrapType";

        case cannon:
            return "Cannon";

        case spike:
            return "Spiketrap";

        case boulder:
            return "Boulder";

        default:
            return "UnknownTrapType";
    }
}

Trap::TrapType Trap::getTrapTypeFromMeshName(std::string s)
{
    if (s.compare("Cannon") == 0)
        return cannon;
    else if (s.compare("Boulder") == 0)
        return boulder;
    else if (s.compare("Spiketrap") == 0)
        return spike;
    else if (s.compare("NullTrapType") == 0)
        return nullTrapType;
    else
    {
        std::cerr
        << "\n\n\nERROR:  Trying to get trap type from unknown mesh name, bailing out.\n";
        std::cerr << "Sourcefile: " << __FILE__ << "\tLine: " << __LINE__
        << "\n\n\n";
        exit(1);
    }
}

int Trap::costPerTile(TrapType t)
{
    switch (t)
    {
        case nullTrapType:
            return 0;

        case cannon:
            return 500;

        case spike:
            return 400;

        case boulder:
            return 500;

        default:
            return 100;
    }
}

void Trap::doUpkeep()
{
    uint32_t i = 0;
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
                        ServerNotification::removeTrapTile, NULL);
                    std::string name = getName();
                    serverNotification->mPacket << name << t;
                    ODServer::getSingleton().queueServerNotification(serverNotification);
                }
                catch (std::bad_alloc&)
                {
                    OD_ASSERT_TRUE(false);
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

    // If no more tiles, the trap is removed
    if (numCoveredTiles() <= 0)
    {
        getGameMap()->removeTrap(this);
        deleteYourself();
        return;
    }

    for(std::vector<Tile*>::iterator it = mCoveredTiles.begin(); it != mCoveredTiles.end(); ++it)
    {
        Tile* tile = *it;
        if(mReloadTimeCounters[tile] > 0)
        {
            mReloadTimeCounters[tile]--;
            continue;
        }
        if(shoot(tile))
        {
            mReloadTimeCounters[tile] = mReloadTime;
        }
    }
}

void Trap::addCoveredTile(Tile* t, double nHP)
{
    mCoveredTiles.push_back(t);
    mTileHP[t] = nHP;
    t->setCoveringTrap(this);
    mReloadTimeCounters[t] = mReloadTime;
}

void Trap::removeCoveredTile(Tile* t)
{
    bool removedTile = false;
    for (std::vector<Tile*>::iterator it = mCoveredTiles.begin(); it != mCoveredTiles.end(); ++it)
    {
        if (t == *it)
        {
            mCoveredTiles.erase(it);
            t->setCoveringTrap(NULL);
            mTileHP.erase(t);
            removedTile = true;
            break;
        }
    }
    if (!removedTile)
        return;

    mReloadTimeCounters.erase(t);
    if(getGameMap()->isServerGameMap())
        return;

    // Destroy the mesh for this tile.
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::destroyTrap;
    request->p = this;
    request->p2 = t;
    RenderManager::queueRenderRequest(request);
}

Tile* Trap::getCoveredTile(int index)
{
    return mCoveredTiles[index];
}

std::vector<Tile*> Trap::getCoveredTiles()
{
    return mCoveredTiles;
}

unsigned int Trap::numCoveredTiles()
{
    return mCoveredTiles.size();
}

void Trap::clearCoveredTiles()
{
    mCoveredTiles.clear();
}

double Trap::getHP(Tile *tile) const
{
    //NOTE: This function is the same as Room::getHP(), consider making a base class to inherit this from.
    if (tile != NULL)
    {
        std::map<Tile*, double>::const_iterator tileSearched = mTileHP.find(tile);
        OD_ASSERT_TRUE(tileSearched != mTileHP.end());
        return tileSearched->second;
    }

    // If the tile given was NULL, we add the total HP of all the tiles in the room and return that.
    double total = 0.0;

    for(std::map<Tile*, double>::const_iterator itr = mTileHP.begin(), end = mTileHP.end();
        itr != end; ++itr)
    {
        total += itr->second;
    }

    return total;
}

double Trap::getDefense() const
{
    return 0;
}

void Trap::takeDamage(GameEntity* attacker, double damage, Tile *tileTakingDamage)
{
    mTileHP[tileTakingDamage] -= damage;
}

std::string Trap::getNameTile(Tile* tile)
{
    return getName() + "_tile_" + Ogre::StringConverter::toString(tile->x)
        + "_" + Ogre::StringConverter::toString(tile->y);
}

void Trap::updateActiveSpots()
{
    // Active spots are handled by the server only
    if(!getGameMap()->isServerGameMap())
        return;

    // For a trap, by default, every tile is an active spot
    if(mCoveredTiles.size() > mRoomObjects.size())
    {
        // More tiles than room objects. This will happen when the trap is created
        for(std::vector<Tile*>::iterator it = mCoveredTiles.begin(); it != mCoveredTiles.end(); ++it)
        {
            Tile* tile = *it;
            RoomObject* obj = notifyActiveSpotCreated(tile);
            if(obj == NULL)
                continue;

            addRoomObject(tile, obj);
        }
    }
    else if(mCoveredTiles.size() < mRoomObjects.size())
    {
        // Less tiles than room objects. This will happen when a tile from this trap is destroyed
        std::vector<Tile*> tilesToRemove;
        for(std::map<Tile*, RoomObject*>::iterator it = mRoomObjects.begin(); it != mRoomObjects.end(); ++it)
        {
            Tile* tile = it->first;
            // We store removed tiles
            if(std::find(mCoveredTiles.begin(), mCoveredTiles.end(), tile) == mCoveredTiles.end())
                tilesToRemove.push_back(tile);
        }

        // Then, we process removing (that will remove tiles from mRoomObjects)
        OD_ASSERT_TRUE(!tilesToRemove.empty());
        for(std::vector<Tile*>::iterator it = tilesToRemove.begin(); it != tilesToRemove.end(); ++it)
        {
            Tile* tile = *it;
            if(mRoomObjects.count(tile) > 0)
                notifyActiveSpotRemoved(tile);
        }
    }
}

RoomObject* Trap::notifyActiveSpotCreated(Tile* tile)
{
    return NULL;
}

void Trap::notifyActiveSpotRemoved(Tile* tile)
{
    removeRoomObject(tile);
}

std::string Trap::getFormat()
{
    return "meshName\tseatId\t\tNextLine: numTiles\t\tSubsequent Lines: tileX\ttileY";
}

std::istream& operator>>(std::istream& is, Trap *t)
{
    // When we read map file, the mesh type is read before building the room. That's
    // why we assume it is already set in the room
    int tilesToLoad, tempX, tempY, tempInt;

    is >> tempInt;
    t->setSeat(t->getGameMap()->getSeatById(tempInt));

    is >> tilesToLoad;
    for (int i = 0; i < tilesToLoad; ++i)
    {
        is >> tempX >> tempY;
        Tile *tempTile = t->getGameMap()->getTile(tempX, tempY);
        if (tempTile != NULL)
        {
            t->addCoveredTile(tempTile);
            tempTile->setSeat(t->getSeat());
        }
    }

    t->mType = Trap::getTrapTypeFromMeshName(t->getMeshName());
    return is;
}

std::ostream& operator<<(std::ostream& os, Trap *t)
{
    int seatId = t->getSeat()->getId();
    os << t->getMeshName() << "\t";
    os << seatId << "\n" << t->mCoveredTiles.size() << "\n";
    for (unsigned int i = 0; i < t->mCoveredTiles.size(); ++i)
    {
        Tile *tempTile = t->mCoveredTiles[i];
        os << tempTile->x << "\t" << tempTile->y << "\n";
    }

    return os;
}

ODPacket& operator>>(ODPacket& is, Trap *t)
{
    // When we read map file, the mesh type is read before building the room. To have the
    // same behaviour, we assume the same and that it is already set in the room
    int tilesToLoad, tempX, tempY, tempInt;

    is >> tempInt;
    t->setSeat(t->getGameMap()->getSeatById(tempInt));

    is >> tilesToLoad;
    for (int i = 0; i < tilesToLoad; ++i)
    {
        is >> tempX >> tempY;
        Tile *tempTile = t->getGameMap()->getTile(tempX, tempY);
        if (tempTile != NULL)
        {
            t->addCoveredTile(tempTile);
            tempTile->setSeat(t->getSeat());
        }
        else
        {
            LogManager::getSingleton().logMessage("ERROR : trying to add trap on unkown tile "
                + Ogre::StringConverter::toString(tempX) + "," + Ogre::StringConverter::toString(tempY));
        }
    }

    t->mType = Trap::getTrapTypeFromMeshName(t->getMeshName());
    return is;
}

ODPacket& operator<<(ODPacket& os, Trap *t)
{
    int nbTiles = t->mCoveredTiles.size();
    std::string meshName = t->getMeshName();
    std::string name = t->getName();
    int seatId = t->getSeat()->getId();
    os << meshName << name << seatId;
    os << nbTiles;
    for(std::vector<Tile*>::iterator it = t->mCoveredTiles.begin(); it != t->mCoveredTiles.end(); ++it)
    {
        Tile *tempTile = *it;
        os << tempTile->x << tempTile->y;
    }

    return os;
}
