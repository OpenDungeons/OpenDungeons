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

#include "rooms/RoomPortalWave.h"

#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/PersistentObject.h"
#include "entities/Tile.h"
#include "gamemap/GameMap.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

RoomPortalWave::RoomPortalWave(GameMap* gameMap) :
        Room(gameMap),
        mSpawnCountdown(0),
        mPortalObject(nullptr),
        mClaimedValue(0)
{
   setMeshName("Portal");
}

RoomPortalWave::~RoomPortalWave()
{
    for(RoomPortalWaveData* roomPortalWaveData : mRoomPortalWaveDataSpawnable)
        delete roomPortalWaveData;

    mRoomPortalWaveDataSpawnable.clear();
    for(RoomPortalWaveData* roomPortalWaveData : mRoomPortalWaveDataNotSpawnable)
        delete roomPortalWaveData;

    mRoomPortalWaveDataNotSpawnable.clear();
}

bool RoomPortalWave::isClaimable(Seat* seat) const
{
    if(getSeat()->canBuildingBeDestroyedBy(seat))
        return false;

    return true;
}

void RoomPortalWave::claimForSeat(Seat* seat, Tile* tile, double danceRate)
{
    if(mClaimedValue > danceRate)
    {
        mClaimedValue-= danceRate;
        return;
    }

    // In the case of RoomPortalWave, when it is claimed, it is destroyed
    for(std::pair<Tile* const, TileData*>& p : mTileData)
        p.second->mHP = 0.0;
}

void RoomPortalWave::updateActiveSpots()
{
    // Room::updateActiveSpots(); <<-- Disabled on purpose.
    // We don't update the active spots the same way as only the central tile is needed.
    if (getGameMap()->isInEditorMode())
        updatePortalPosition();
    else
    {
        if(mPortalObject == nullptr)
        {
            // We check if the portal already exists (that can happen if it has
            // been restored after restoring a saved game)
            if(mBuildingObjects.empty())
                updatePortalPosition();
            else
            {
                for(std::pair<Tile* const, RenderedMovableEntity*>& p : mBuildingObjects)
                {
                    if(p.second == nullptr)
                        continue;

                    // We take the first RenderedMovableEntity. Note that we cannot use
                    // the central tile because after saving a game, the central tile may
                    // not be the same if some tiles have been destroyed
                    mPortalObject = p.second;
                    break;
                }
            }
        }
    }
}

void RoomPortalWave::updatePortalPosition()
{
    // Only the server game map should load objects.
    if (!getGameMap()->isServerGameMap())
        return;

    // Delete all previous rooms meshes and recreate a central one.
    removeAllBuildingObjects();
    mPortalObject = nullptr;

    Tile* centralTile = getCentralTile();
    if (centralTile == nullptr)
        return;

    mPortalObject = new PersistentObject(getGameMap(), getName(), "KnightCoffin", centralTile, 0.0, false);
    addBuildingObject(centralTile, mPortalObject);

    mPortalObject->setAnimationState("Idle");
}

void RoomPortalWave::destroyMeshLocal()
{
    Room::destroyMeshLocal();
    mPortalObject = nullptr;
}

void RoomPortalWave::doUpkeep()
{
    // Call the super class Room::doUpkeep() function to do any generic upkeep common to all rooms.
    Room::doUpkeep();

    if (mCoveredTiles.empty())
        return;

    if (mSpawnCountdown < mTurnsBetween2Waves)
    {
        ++mSpawnCountdown;
        mPortalObject->setAnimationState("Idle");
        return;
    }

    mSpawnCountdown = 0;

    // We start by checking that the spawnable wave list is up to date
    int64_t curTurn = getGameMap()->getTurnNumber();
    for(auto it = mRoomPortalWaveDataSpawnable.begin(); it != mRoomPortalWaveDataSpawnable.end();)
    {
        RoomPortalWaveData* roomPortalWaveData = *it;
        if(roomPortalWaveData->mSpawnTurnMin > curTurn)
        {
            // Cannot spawn the wave anymore
            it = mRoomPortalWaveDataSpawnable.erase(it);
            mRoomPortalWaveDataNotSpawnable.push_back(roomPortalWaveData);
            continue;
        }

        if((roomPortalWaveData->mSpawnTurnMax != -1) &&
           (roomPortalWaveData->mSpawnTurnMax < curTurn))
        {
            it = mRoomPortalWaveDataSpawnable.erase(it);
            mRoomPortalWaveDataNotSpawnable.push_back(roomPortalWaveData);
            continue;
        }

        ++it;
    }

    // Now we check in the not spawnable list if a wave is available
    for(auto it = mRoomPortalWaveDataNotSpawnable.begin(); it != mRoomPortalWaveDataNotSpawnable.end();)
    {
        RoomPortalWaveData* roomPortalWaveData = *it;
        if(roomPortalWaveData->mSpawnTurnMin > curTurn)
        {
            ++it;
            continue;
        }

        if((roomPortalWaveData->mSpawnTurnMax != -1) &&
           (roomPortalWaveData->mSpawnTurnMax < curTurn))
        {
            ++it;
            continue;
        }

        // The wave is spawnable
        it = mRoomPortalWaveDataNotSpawnable.erase(it);
        mRoomPortalWaveDataSpawnable.push_back(roomPortalWaveData);
    }

    if(mRoomPortalWaveDataSpawnable.empty())
        return;

    // Portal waves is not concerned about creature limit per seat. Only by the absolute limit
    uint32_t maxCreatures = ConfigManager::getSingleton().getMaxCreaturesPerSeatAbsolute();
    uint32_t numCreatures = getGameMap()->getNbFightersForSeat(getSeat());

    if(numCreatures >= maxCreatures)
        return;

    // Randomly choose a wave to spawn
    uint32_t index = Random::Uint(0, mRoomPortalWaveDataSpawnable.size() - 1);
    spawnWave(mRoomPortalWaveDataSpawnable[index], maxCreatures - numCreatures);

}

void RoomPortalWave::spawnWave(RoomPortalWaveData* roomPortalWaveData, uint32_t maxCreaturesToSpawn)
{
    Tile* centralTile = getCentralTile();
    if (centralTile == nullptr)
        return;

    if (mPortalObject != nullptr)
        mPortalObject->setAnimationState("Triggered", false);

    Ogre::Real xPos = static_cast<Ogre::Real>(centralTile->getX());
    Ogre::Real yPos = static_cast<Ogre::Real>(centralTile->getY());
    Ogre::Vector3 spawnPosition(xPos, yPos, 0.0f);

    // Create the wave
    for(const std::pair<std::string, uint32_t>& p : roomPortalWaveData->mSpawnCreatureClassName)
    {
        if(maxCreaturesToSpawn <= 0)
            break;

        const CreatureDefinition* classToSpawn = getGameMap()->getClassDescription(p.first);
        if(classToSpawn == nullptr)
        {
            OD_ASSERT_TRUE_MSG(false, "RoomPortalWave=" + getName() + ", wrong creature class=" + p.first);
            continue;
        }

        Creature* newCreature = new Creature(getGameMap(), classToSpawn);
        newCreature->setLevel(p.second);

        LogManager::getSingleton().logMessage("RoomPortalWave name=" + getName()
            + "spawns a creature class=" + classToSpawn->getClassName()
            + ", name=" + newCreature->getName() + ", seatId=" + Helper::toString(getSeat()->getId()));

        newCreature->setSeat(getSeat());
        newCreature->addToGameMap();
        newCreature->createMesh();
        newCreature->setPosition(spawnPosition, false);

        ++maxCreaturesToSpawn;
    }
}

void RoomPortalWave::setupRoom(const std::string& name, Seat* seat, const std::vector<Tile*>& tiles)
{
    Room::setupRoom(name, seat, tiles);
    mClaimedValue = static_cast<double>(numCoveredTiles());
}

void RoomPortalWave::exportToStream(std::ostream& os) const
{
    Room::exportToStream(os);

    os << mClaimedValue << "\n" << mTurnsBetween2Waves << "\n";
    os << "[Waves]" << "\n";
    for(RoomPortalWaveData* roomPortalWaveData : mRoomPortalWaveDataNotSpawnable)
    {
        os << "[Wave]" << "\n";
        os << "SpawnTurnMin\t" << roomPortalWaveData->mSpawnTurnMin << "\n";
        os << "SpawnTurnMax\t" << roomPortalWaveData->mSpawnTurnMax << "\n";
        os << "[Creatures]" << "\n";
        for(const std::pair<std::string, uint32_t>& p : roomPortalWaveData->mSpawnCreatureClassName)
        {
            os << p.first << "\t" << p.second << "\n";
        }
        os << "[/Creatures]" << "\n";
        os << "[/Wave]" << "\n";
    }
    for(RoomPortalWaveData* roomPortalWaveData : mRoomPortalWaveDataSpawnable)
    {
        os << "[Wave]" << "\n";
        os << "SpawnTurnMin\t" << roomPortalWaveData->mSpawnTurnMin << "\n";
        os << "SpawnTurnMax\t" << roomPortalWaveData->mSpawnTurnMax << "\n";
        os << "[Creatures]" << "\n";
        for(const std::pair<std::string, uint32_t>& p : roomPortalWaveData->mSpawnCreatureClassName)
        {
            os << p.first << "\t" << p.second << "\n";
        }
        os << "[/Creatures]" << "\n";
        os << "[/Wave]" << "\n";
    }
    os << "[/Waves]" << "\n";
}

void RoomPortalWave::importFromStream(std::istream& is)
{
    Room::importFromStream(is);

    OD_ASSERT_TRUE(is >> mClaimedValue);
    OD_ASSERT_TRUE(is >> mTurnsBetween2Waves);
    std::string str;
    OD_ASSERT_TRUE(is >> str);
    if(str != "[Waves]")
    {
        OD_ASSERT_TRUE_MSG(false, "RoomPortalWave=" + getName() + ", str=" + str);
        return;
    }
    while(true)
    {
        OD_ASSERT_TRUE(is >> str);
        if(str == "[/Waves]")
            break;

        if(str != "[Wave]")
        {
            OD_ASSERT_TRUE_MSG(false, "RoomPortalWave=" + getName() + ", str=" + str);
            return;
        }

        RoomPortalWaveData* roomPortalWaveData = new RoomPortalWaveData;
        OD_ASSERT_TRUE(is >> str);
        if(str != "SpawnTurnMin")
        {
            OD_ASSERT_TRUE_MSG(false, "RoomPortalWave=" + getName() + ", str=" + str);
            delete roomPortalWaveData;
            return;
        }
        OD_ASSERT_TRUE(is >> roomPortalWaveData->mSpawnTurnMin);

        OD_ASSERT_TRUE(is >> str);
        if(str != "SpawnTurnMax")
        {
            OD_ASSERT_TRUE_MSG(false, "RoomPortalWave=" + getName() + ", str=" + str);
            delete roomPortalWaveData;
            return;
        }
        OD_ASSERT_TRUE(is >> roomPortalWaveData->mSpawnTurnMax);

        OD_ASSERT_TRUE(is >> str);
        if(str != "[Creatures]")
        {
            OD_ASSERT_TRUE_MSG(false, "RoomPortalWave=" + getName() + ", str=" + str);
            delete roomPortalWaveData;
            return;
        }

        while(true)
        {
            OD_ASSERT_TRUE(is >> str);
            if(str == "[/Creatures]")
                break;

            std::pair<std::string, uint32_t> p;
            p.first = str;
            OD_ASSERT_TRUE(is >> p.second);

            roomPortalWaveData->mSpawnCreatureClassName.push_back(p);
        }

        OD_ASSERT_TRUE(is >> str);
        if(str != "[/Wave]")
        {
            OD_ASSERT_TRUE_MSG(false, "RoomPortalWave=" + getName() + ", str=" + str);
            delete roomPortalWaveData;
            return;
        }

        addRoomPortalWaveData(roomPortalWaveData);
    }
}

void RoomPortalWave::restoreInitialEntityState()
{
    // We need to use seats with vision before calling Room::restoreInitialEntityState
    // because it will empty the list
    if(mPortalObject == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "RoomPortalWave=" + getName());
        return;
    }

    Tile* tilePortalObject = mPortalObject->getPositionTile();
    if(tilePortalObject == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "RoomPortalWave=" + getName() + ", mPortalObject=" + mPortalObject->getName());
        return;
    }
    TileData* tileData = mTileData[tilePortalObject];
    if(tileData == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "RoomPortalWave=" + getName() + ", tile=" + Tile::displayAsString(tilePortalObject));
        return;
    }

    if(!tileData->mSeatsVision.empty())
        mPortalObject->notifySeatsWithVision(tileData->mSeatsVision);

    // If there are no covered tile, the temple object is not working
    if(numCoveredTiles() == 0)
        mPortalObject->notifyRemoveAsked();

    Room::restoreInitialEntityState();
}

void RoomPortalWave::addRoomPortalWaveData(RoomPortalWaveData* roomPortalWaveData)
{
    int64_t curTurn = getGameMap()->getTurnNumber();
    if((roomPortalWaveData->mSpawnTurnMin >= curTurn) &&
       (roomPortalWaveData->mSpawnTurnMax <= curTurn))
    {
        mRoomPortalWaveDataSpawnable.push_back(roomPortalWaveData);
    }
    else
    {
        mRoomPortalWaveDataNotSpawnable.push_back(roomPortalWaveData);
    }
}
