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

#include "rooms/RoomTreasury.h"

#include "gamemap/GameMap.h"
#include "entities/Tile.h"
#include "entities/RenderedMovableEntity.h"
#include "utils/LogManager.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "sound/SoundEffectsManager.h"

#include <string>

static const int maxGoldinTile = 5000;

RoomTreasury::RoomTreasury(GameMap* gameMap) :
    Room(gameMap),
    mGoldChanged(false)
{
    setMeshName("Treasury");
}

void RoomTreasury::doUpkeep()
{
    Room::doUpkeep();

    if (mCoveredTiles.empty())
        return;

    if(mGoldChanged)
    {
        for (std::pair<Tile* const, int>& p : mGoldInTile)
        {
            Tile* tile = p.first;
            updateMeshesForTile(tile);
        }
        mGoldChanged = false;
    }
}

void RoomTreasury::absorbRoom(Room *r)
{
    RoomTreasury* rt = static_cast<RoomTreasury*>(r);
    for(std::pair<Tile* const, int>& p : rt->mGoldInTile)
    {
        Tile* tile = p.first;
        int gold = p.second;
        mGoldInTile[tile] = gold;
    }
    rt->mGoldInTile.clear();

    for(std::pair<Tile* const, std::string>& p : rt->mMeshOfTile)
    {
        Tile* tile = p.first;
        mMeshOfTile[tile] = p.second;
    }
    rt->mMeshOfTile.clear();

    Room::absorbRoom(r);
}

void RoomTreasury::addCoveredTile(Tile* t, double nHP)
{
    Room::addCoveredTile(t, nHP);

    // Only initialize the tile to empty if it has not already been set by the absorbRoom() function.
    if (mGoldInTile.find(t) == mGoldInTile.end())
    {
        mGoldInTile[t] = 0;
        mMeshOfTile[t] = std::string();
    }
}

bool RoomTreasury::removeCoveredTile(Tile* t)
{
    // if the mesh has gold, we erase the mesh
    if((mMeshOfTile.count(t) > 0) && (!mMeshOfTile[t].empty()))
        removeBuildingObject(t);

    if(mGoldInTile.count(t) > 0)
    {
        int value = mGoldInTile[t];
        if(getGameMap()->isServerGameMap() && (value > 0))
        {
            LogManager::getSingleton().logMessage("Room " + getName()
                + ", tile=" + Tile::displayAsString(t) + " releases gold amount = "
                + Ogre::StringConverter::toString(value));
            TreasuryObject* obj = new TreasuryObject(getGameMap(), value);
            obj->addToGameMap();
            Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(t->getX()),
                                        static_cast<Ogre::Real>(t->getY()), 0.0f);
            obj->createMesh();
            obj->setPosition(spawnPosition, false);
        }
        mGoldInTile.erase(t);
    }
    mMeshOfTile.erase(t);

    return Room::removeCoveredTile(t);
}

int RoomTreasury::getTotalGold()
{
    int tempInt = 0;

    for (std::pair<Tile* const, int>& p : mGoldInTile)
        tempInt += p.second;

    return tempInt;
}

int RoomTreasury::emptyStorageSpace()
{
    return numCoveredTiles() * maxGoldinTile - getTotalGold();
}

int RoomTreasury::depositGold(int gold, Tile *tile)
{
    int goldDeposited, goldToDeposit = gold, emptySpace;

    // Start by trying to deposit the gold in the requested tile.
    emptySpace = maxGoldinTile - mGoldInTile[tile];
    goldDeposited = std::min(emptySpace, goldToDeposit);
    mGoldInTile[tile] += goldDeposited;
    goldToDeposit -= goldDeposited;

    // If there is still gold left to deposit after the first tile, loop over all of the tiles and see if we can put the gold in another tile.
    for (std::pair<Tile* const, int>& p : mGoldInTile)
    {
        if(goldToDeposit <= 0)
            break;

        // Store as much gold as we can in this tile.
        emptySpace = maxGoldinTile - p.second;
        goldDeposited = std::min(emptySpace, goldToDeposit);
        p.second += goldDeposited;
        goldToDeposit -= goldDeposited;
    }

    // Return the amount we were actually able to deposit
    // (i.e. the amount we wanted to deposit minus the amount we were unable to deposit).
    int wasDeposited = gold - goldToDeposit;
    // If we couldn't deposit anything, we do not notify
    if(wasDeposited == 0)
        return wasDeposited;

    mGoldChanged = true;

    if(getGameMap()->isServerGameMap() == false)
        return wasDeposited;

    // Tells the client to play a deposit gold sound. For now, we only send it to the players
    // with vision on tile
    for(Seat* seat : getGameMap()->getSeats())
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;
        if(!seat->hasVisionOnTile(tile))
            continue;

        ServerNotification *serverNotification = new ServerNotification(
            ServerNotification::playSpatialSound, nullptr);
        serverNotification->mPacket << SoundEffectsManager::DEPOSITGOLD << tile->getX() << tile->getY();
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }

    return wasDeposited;
}

int RoomTreasury::withdrawGold(int gold)
{
    mGoldChanged = true;

    int withdrawlAmount = 0;
    for (std::pair<Tile* const, int>& p : mGoldInTile)
    {
        // Check to see if the current room tile has enough gold in it to fill the amount we still need to pick up.
        int goldStillNeeded = gold - withdrawlAmount;
        if (p.second > goldStillNeeded)
        {
            // There is enough to satisfy the request so we do so and exit the loop.
            withdrawlAmount += goldStillNeeded;
            p.second -= goldStillNeeded;
            break;
        }
        else
        {
            // There is not enough to satisfy the request so take everything there is and move on to the next tile.
            withdrawlAmount += p.second;
            p.second = 0;
        }
    }

    return withdrawlAmount;
}

void RoomTreasury::updateMeshesForTile(Tile* t)
{
    if(mGoldInTile.count(t) == 0)
        return;

    int gold = mGoldInTile[t];
    OD_ASSERT_TRUE_MSG(gold <= maxGoldinTile, "room=" + getName() + ", gold=" + Ogre::StringConverter::toString(gold));

    // If the mesh has not changed we do not need to do anything.
    std::string newMeshName = TreasuryObject::getMeshNameForGold(gold);
    if (mMeshOfTile[t].compare(newMeshName) == 0)
        return;

    // If the mesh has changed we need to destroy the existing treasury if there was one
    if (!mMeshOfTile[t].empty())
        removeBuildingObject(t);

    if (gold > 0)
    {
        RenderedMovableEntity* ro = loadBuildingObject(getGameMap(), newMeshName, t, 0.0, false);
        addBuildingObject(t, ro);
    }

    mMeshOfTile[t] = newMeshName;
}

bool RoomTreasury::hasCarryEntitySpot(MovableGameEntity* carriedEntity)
{
    // We might accept more gold than empty space (for example, if there are 100 gold left
    // and 2 different workers want to bring a treasury) but we don't care
    if(carriedEntity->getObjectType() != GameEntity::ObjectType::renderedMovableEntity)
        return false;

    RenderedMovableEntity* renderedEntity = static_cast<RenderedMovableEntity*>(carriedEntity);
    if(renderedEntity->getRenderedMovableEntityType() != RenderedMovableEntity::RenderedMovableEntityType::treasuryObject)
        return false;

    if(emptyStorageSpace() <= 0)
        return false;

    return true;
}

Tile* RoomTreasury::askSpotForCarriedEntity(MovableGameEntity* carriedEntity)
{
    if(!hasCarryEntitySpot(carriedEntity))
        return nullptr;

    return mCoveredTiles[0];
}

void RoomTreasury::notifyCarryingStateChanged(Creature* carrier, MovableGameEntity* carriedEntity)
{
    // If a treasury is deposited on the treasury, no need to handle it here.
    // It will handle himself alone
}
