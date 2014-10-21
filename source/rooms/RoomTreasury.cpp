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

#include "rooms/RoomTreasury.h"

#include "gamemap/GameMap.h"
#include "entities/Tile.h"
#include "entities/RenderedMovableEntity.h"
#include "utils/LogManager.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "entities/TreasuryObject.h"
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
        for (std::map<Tile*, int>::iterator itr = mGoldInTile.begin(); itr != mGoldInTile.end(); ++itr)
        {
            Tile* tile = itr->first;
            updateMeshesForTile(tile);
        }
        mGoldChanged = false;
    }
}

void RoomTreasury::absorbRoom(Room *r)
{
    RoomTreasury* rt = static_cast<RoomTreasury*>(r);
    for(std::map<Tile*, int>::iterator it = rt->mGoldInTile.begin(); it != rt->mGoldInTile.end(); ++it)
    {
        Tile* tile = it->first;
        int gold = it->second;
        mGoldInTile[tile] = gold;
    }
    rt->mGoldInTile.clear();

    for(std::map<Tile*, TreasuryTileFullness>::iterator it = rt->mFullnessOfTile.begin(); it != rt->mFullnessOfTile.end(); ++it)
    {
        Tile* tile = it->first;
        TreasuryTileFullness fullness = it->second;
        mFullnessOfTile[tile] = fullness;
    }
    rt->mFullnessOfTile.clear();

    Room::absorbRoom(r);
}

void RoomTreasury::addCoveredTile(Tile* t, double nHP, bool isRoomAbsorb)
{
    Room::addCoveredTile(t, nHP, isRoomAbsorb);

    // Only initialize the tile to empty if it has not already been set by the absorbRoom() function.
    if (mGoldInTile.find(t) == mGoldInTile.end())
    {
        mGoldInTile[t] = 0;
        mFullnessOfTile[t] = noGold;
    }
}

bool RoomTreasury::removeCoveredTile(Tile* t, bool isRoomAbsorb)
{
    // if the mesh has gold, we erase the mesh
    if((mFullnessOfTile.count(t) > 0) && (mFullnessOfTile[t] != noGold))
        removeBuildingObject(t);

    if(mGoldInTile.count(t) > 0)
    {
        int value = mGoldInTile[t];
        if(getGameMap()->isServerGameMap() && (value > 0))
        {
            LogManager::getSingleton().logMessage("Room " + getName()
                + ", tile=" + Tile::displayAsString(t) + " releases gold amount = "
                + Ogre::StringConverter::toString(value));
            TreasuryObject* to = new TreasuryObject(getGameMap(), value);
            Ogre::Vector3 pos(static_cast<Ogre::Real>(t->x), static_cast<Ogre::Real>(t->y), 0.0f);
            to->setPosition(pos);
            getGameMap()->addRenderedMovableEntity(to);
        }
        mGoldInTile.erase(t);
    }
    mFullnessOfTile.erase(t);

    //TODO:  When the tile contains gold we need to put it on the map as an item which can be picked up.
    return Room::removeCoveredTile(t, isRoomAbsorb);
}

int RoomTreasury::getTotalGold()
{
    int tempInt = 0;

    for (std::map<Tile*, int>::iterator itr = mGoldInTile.begin();
            itr != mGoldInTile.end(); ++itr)
        tempInt += (*itr).second;

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
    for (std::map<Tile*, int>::iterator itr = mGoldInTile.begin(); itr != mGoldInTile.end() && goldToDeposit > 0; ++itr)
    {
        // Store as much gold as we can in this tile.
        emptySpace = maxGoldinTile - itr->second;
        goldDeposited = std::min(emptySpace, goldToDeposit);
        itr->second += goldDeposited;
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

    // Tells the client to play a deposit gold sound
    try
    {
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotification::playSpatialSound, NULL);
        serverNotification->mPacket << static_cast<int>(SoundEffectsManager::DEPOSITGOLD) << tile->getX() << tile->getY();
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
    catch (std::bad_alloc&)
    {
        Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in RoomTreasury::depositGold", Ogre::LML_CRITICAL);
        exit(1);
    }

    return wasDeposited;
}

int RoomTreasury::withdrawGold(int gold)
{
    mGoldChanged = true;

    int withdrawlAmount = 0;
    for (std::map<Tile*, int>::iterator itr = mGoldInTile.begin(); itr != mGoldInTile.end(); ++itr)
    {
        // Check to see if the current room tile has enough gold in it to fill the amount we still need to pick up.
        int goldStillNeeded = gold - withdrawlAmount;
        if (itr->second > goldStillNeeded)
        {
            // There is enough to satisfy the request so we do so and exit the loop.
            withdrawlAmount += goldStillNeeded;
            itr->second -= goldStillNeeded;
            break;
        }
        else
        {
            // There is not enough to satisfy the request so take everything there is and move on to the next tile.
            withdrawlAmount += itr->second;
            itr->second = 0;
        }
    }

    return withdrawlAmount;
}

RoomTreasury::TreasuryTileFullness RoomTreasury::getTreasuryTileFullness(int gold)
{
    if (gold <= 0)
        return noGold;
    if (gold <= maxGoldinTile / 4)
        return quarter;
    if (gold <= maxGoldinTile / 2)
        return half;
    if (gold <= maxGoldinTile / 4 * 3)
        return threeQuarters;

    if (gold > maxGoldinTile)
    {
        std::stringstream str("");
        str << "Warning invalid amount of gold on treasury tile (" << gold
            << "/" << maxGoldinTile << ")";
        LogManager::getSingleton().logMessage(str.str());
    }

    return fullOfGold;
}

const char* RoomTreasury::getMeshNameForTreasuryTileFullness(TreasuryTileFullness fullness)
{
    switch (fullness)
    {
        case quarter:
            return "GoldstackLv1";

        case half:
            return "GoldstackLv2";

        case threeQuarters:
            return "GoldstackLv3";

        case fullOfGold:
            return "GoldstackLv4";

        // The empty case should really never happen since we shouldn't be creating meshes for an empty tile anyway.
        case noGold:
        default:
            return "TreasuryTileFullnessMeshError";
    }
}

void RoomTreasury::updateMeshesForTile(Tile* t)
{
    if(mGoldInTile.count(t) == 0)
        return;

    TreasuryTileFullness newFullness = getTreasuryTileFullness(mGoldInTile[t]);

    // If the mesh has not changed we do not need to do anything.
    if (mFullnessOfTile[t] == newFullness)
        return;

    // If the fullness level has changed we need to destroy the existing treasury
    if (mFullnessOfTile[t] != noGold)
        removeBuildingObject(t);

    if (newFullness != noGold)
    {
        RenderedMovableEntity* ro = loadBuildingObject(getGameMap(), getMeshNameForTreasuryTileFullness(newFullness), t, 0.0);
        addBuildingObject(t, ro);
    }

    mFullnessOfTile[t] = newFullness;
}
