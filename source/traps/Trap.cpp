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

#include "traps/Trap.h"

#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "entities/Tile.h"
#include "render/RenderRequest.h"
#include "render/RenderManager.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "entities/RenderedMovableEntity.h"
#include "traps/TrapCannon.h"
#include "traps/TrapSpike.h"
#include "traps/TrapBoulder.h"
#include "utils/Random.h"
#include "game/Player.h"
#include "utils/LogManager.h"

Trap::Trap(GameMap* gameMap) :
    Building(gameMap),
    mReloadTime(0),
    mMinDamage(0.0),
    mMaxDamage(0.0)
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
        RenderRequest* request = new RenderRequestCreateBuilding(this, coveredTiles[i]);
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
        RenderRequest *request = new RenderRequestDestroyBuilding(this, coveredTiles[i]);
        RenderManager::queueRenderRequest(request);
    }
}

Trap* Trap::getTrapFromStream(GameMap* gameMap, std::istream &is)
{
    Trap* tempTrap = nullptr;
    TrapType nType;
    is >> nType;

    switch (nType)
    {
        case nullTrapType:
            tempTrap = nullptr;
            break;
        case cannon:
            tempTrap = TrapCannon::getTrapCannonFromStream(gameMap, is);
            break;
        case spike:
            tempTrap = TrapSpike::getTrapSpikeFromStream(gameMap, is);
            break;
        case boulder:
            tempTrap = TrapBoulder::getTrapBoulderFromStream(gameMap, is);
            break;
        default:
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(nType)));
    }

    if(tempTrap == nullptr)
        return nullptr;

    tempTrap->importFromStream(is);

    return tempTrap;
}

Trap* Trap::getTrapFromPacket(GameMap* gameMap, ODPacket &is)
{
    Trap* tempTrap = nullptr;
    TrapType nType;
    is >> nType;

    switch (nType)
    {
        case nullTrapType:
            tempTrap = nullptr;
            break;
        case cannon:
            tempTrap = TrapCannon::getTrapCannonFromPacket(gameMap, is);
            break;
        case spike:
            tempTrap = TrapSpike::getTrapSpikeFromPacket(gameMap, is);
            break;
        case boulder:
            tempTrap = TrapBoulder::getTrapBoulderFromPacket(gameMap, is);
            break;
        default:
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(nType)));
    }

    if(tempTrap == nullptr)
        return nullptr;

    tempTrap->importFromPacket(is);

    return tempTrap;
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
        LogManager::getSingleton().logMessage("Removing trap " + getName());
        getGameMap()->removeTrap(this);
        deleteYourself();
        return;
    }

    for(std::vector<Tile*>::iterator it = mCoveredTiles.begin(); it != mCoveredTiles.end(); ++it)
    {
        Tile* tile = *it;
        if(mReloadTimeCounters[tile] > 0)
        {
            --mReloadTimeCounters[tile];
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
    Building::addCoveredTile(t, nHP);
    t->setCoveringTrap(this);
    mReloadTimeCounters[t] = mReloadTime;
}

bool Trap::removeCoveredTile(Tile* t)
{
    if(!Building::removeCoveredTile(t))
        return false;

    t->setCoveringTrap(NULL);
    mReloadTimeCounters.erase(t);
    if(getGameMap()->isServerGameMap())
        return true;

    // Destroy the mesh for this tile.
    RenderRequest *request = new RenderRequestDestroyBuilding(this, t);
    RenderManager::queueRenderRequest(request);
    return true;
}

void Trap::updateActiveSpots()
{
    // Active spots are handled by the server only
    if(!getGameMap()->isServerGameMap())
        return;

    // For a trap, by default, every tile is an active spot
    if(mCoveredTiles.size() > mBuildingObjects.size())
    {
        // More tiles than RenderedMovableEntity. This will happen when the trap is created
        for(std::vector<Tile*>::iterator it = mCoveredTiles.begin(); it != mCoveredTiles.end(); ++it)
        {
            Tile* tile = *it;
            RenderedMovableEntity* obj = notifyActiveSpotCreated(tile);
            if(obj == NULL)
                continue;

            addBuildingObject(tile, obj);
        }
    }
    else if(mCoveredTiles.size() < mBuildingObjects.size())
    {
        // Less tiles than RenderedMovableEntity. This will happen when a tile from this trap is destroyed
        std::vector<Tile*> tilesToRemove;
        for(std::map<Tile*, RenderedMovableEntity*>::iterator it = mBuildingObjects.begin(); it != mBuildingObjects.end(); ++it)
        {
            Tile* tile = it->first;
            // We store removed tiles
            if(std::find(mCoveredTiles.begin(), mCoveredTiles.end(), tile) == mCoveredTiles.end())
                tilesToRemove.push_back(tile);
        }

        // Then, we process removing (that will remove tiles from mBuildingObjects)
        OD_ASSERT_TRUE(!tilesToRemove.empty());
        for(std::vector<Tile*>::iterator it = tilesToRemove.begin(); it != tilesToRemove.end(); ++it)
        {
            Tile* tile = *it;
            if(mBuildingObjects.count(tile) > 0)
                notifyActiveSpotRemoved(tile);
        }
    }
}

RenderedMovableEntity* Trap::notifyActiveSpotCreated(Tile* tile)
{
    return NULL;
}

void Trap::notifyActiveSpotRemoved(Tile* tile)
{
    removeBuildingObject(tile);
}

void Trap::setupTrap(const std::string& name, Seat* seat, const std::vector<Tile*>& tiles)
{
    setName(name);
    setSeat(seat);
    for(std::vector<Tile*>::const_iterator it = tiles.begin(); it != tiles.end(); ++it)
    {
        Tile* tile = *it;
        addCoveredTile(tile, Trap::DEFAULT_TILE_HP);
    }
}

std::string Trap::getFormat()
{
    return "typeTrap\tseatId\tnumTiles\t\tSubsequent Lines: tileX\ttileY\t\tSubsequent Lines: optional specific data";
}

void Trap::exportHeadersToStream(std::ostream& os)
{
    os << getType() << "\t";
}

void Trap::exportHeadersToPacket(ODPacket& os)
{
    os << getType();
}

void Trap::exportToPacket(ODPacket& os)
{
    int nbTiles = mCoveredTiles.size();
    const std::string& name = getName();
    int seatId = getSeat()->getId();
    os << name << seatId;
    os << nbTiles;
    for(std::vector<Tile*>::iterator it = mCoveredTiles.begin(); it != mCoveredTiles.end(); ++it)
    {
        Tile *tempTile = *it;
        os << tempTile->x << tempTile->y;
    }
}

void Trap::importFromPacket(ODPacket& is)
{
    int tilesToLoad, tempX, tempY, tempInt;
    std::string name;
    OD_ASSERT_TRUE(is >> name);
    setName(name);

    OD_ASSERT_TRUE(is >> tempInt);
    setSeat(getGameMap()->getSeatById(tempInt));

    OD_ASSERT_TRUE(is >> tilesToLoad);
    for (int i = 0; i < tilesToLoad; ++i)
    {
        OD_ASSERT_TRUE(is >> tempX >> tempY);
        Tile *tempTile = getGameMap()->getTile(tempX, tempY);
        OD_ASSERT_TRUE_MSG(tempTile != NULL, "tile=" + Ogre::StringConverter::toString(tempX) + "," + Ogre::StringConverter::toString(tempY));
        if (tempTile != NULL)
        {
            addCoveredTile(tempTile, Trap::DEFAULT_TILE_HP);
            tempTile->setSeat(getSeat());
        }
    }
}

void Trap::exportToStream(std::ostream& os)
{
    int32_t nbTiles = mCoveredTiles.size();
    int seatId = getSeat()->getId();
    os << seatId << "\t" << nbTiles << "\n";
    for(std::vector<Tile*>::iterator it = mCoveredTiles.begin(); it != mCoveredTiles.end(); ++it)
    {
        Tile *tempTile = *it;
        os << tempTile->x << "\t" << tempTile->y << "\n";
    }
}

void Trap::importFromStream(std::istream& is)
{
    int tilesToLoad, tempX, tempY, tempInt;

    OD_ASSERT_TRUE(is >> tempInt);
    setSeat(getGameMap()->getSeatById(tempInt));

    OD_ASSERT_TRUE(is >> tilesToLoad);
    for (int i = 0; i < tilesToLoad; ++i)
    {
        OD_ASSERT_TRUE(is >> tempX >> tempY);
        Tile *tempTile = getGameMap()->getTile(tempX, tempY);
        OD_ASSERT_TRUE_MSG(tempTile != NULL, "tile=" + Ogre::StringConverter::toString(tempX) + "," + Ogre::StringConverter::toString(tempY));
        if (tempTile != NULL)
        {
            addCoveredTile(tempTile, Trap::DEFAULT_TILE_HP);
            tempTile->setSeat(getSeat());
        }
    }
}

std::istream& operator>>(std::istream& is, Trap::TrapType& tt)
{
    uint32_t tmp;
    is >> tmp;
    tt = static_cast<Trap::TrapType>(tmp);
    return is;
}

std::ostream& operator<<(std::ostream& os, const Trap::TrapType& tt)
{
    uint32_t tmp = static_cast<uint32_t>(tt);
    os << tmp;
    return os;
}

ODPacket& operator>>(ODPacket& is, Trap::TrapType& tt)
{
    uint32_t tmp;
    is >> tmp;
    tt = static_cast<Trap::TrapType>(tmp);
    return is;
}

ODPacket& operator<<(ODPacket& os, const Trap::TrapType& tt)
{
    uint32_t tmp = static_cast<uint32_t>(tt);
    os << tmp;
    return os;
}
