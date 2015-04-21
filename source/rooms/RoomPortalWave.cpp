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
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/Pathfinding.h"
#include "gamemap/GameMap.h"
#include "spell/SpellCallToWar.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

#include <vector>

class TileSearch
{
public:
    TileSearch() :
        mTile(nullptr),
        mRotationClockWise(false),
        mRotationCounterClockWise(false)
    {}

    static std::pair<int,int> computeNextRotation(const std::pair<int,int>& currentRotation,
        bool rotationClockWise)
    {
        if(rotationClockWise)
        {
            int newY = currentRotation.first;
            int newX = currentRotation.second * -1;
            return std::pair<int, int>(newX, newY);
        }
        else
        {
            int newY = currentRotation.first * -1;
            int newX = currentRotation.second;
            return std::pair<int, int>(newX, newY);
        }
    }

    Tile* mTile;
    bool mRotationClockWise;
    bool mRotationCounterClockWise;
    std::pair<int,int> mDirectionHit;
};

const double CLAIMED_VALUE_PER_TILE = 1.0;

RoomPortalWave::RoomPortalWave(GameMap* gameMap) :
        Room(gameMap),
        mSpawnCountdown(0),
        mSearchFoeCountdown(0),
        mTurnsBetween2Waves(0),
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

void RoomPortalWave::absorbRoom(Room *r)
{
    RoomPortalWave* oldRoom = static_cast<RoomPortalWave*>(r);
    mClaimedValue += oldRoom->mClaimedValue;
    // We keep the number of creatures increased by this portal
    mTurnsBetween2Waves = oldRoom->mTurnsBetween2Waves;
    mRoomPortalWaveDataSpawnable = oldRoom->mRoomPortalWaveDataSpawnable;
    oldRoom->mRoomPortalWaveDataSpawnable.clear();
    mRoomPortalWaveDataNotSpawnable = oldRoom->mRoomPortalWaveDataNotSpawnable;
    oldRoom->mRoomPortalWaveDataNotSpawnable.clear();

    Room::absorbRoom(r);
}

bool RoomPortalWave::removeCoveredTile(Tile* t)
{
    if(mClaimedValue > CLAIMED_VALUE_PER_TILE)
        mClaimedValue -= CLAIMED_VALUE_PER_TILE;

    return Room::removeCoveredTile(t);
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

    if(mSearchFoeCountdown > 0)
        --mSearchFoeCountdown;
    else
    {
        mSearchFoeCountdown = Random::Uint(10, 20);
        if(!handleSearchFoe())
            handleDigging();
    }

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

        Creature* newCreature = new Creature(getGameMap(), classToSpawn, getSeat(), spawnPosition);
        newCreature->setLevel(p.second);
        newCreature->setHP(newCreature->getMaxHp());

        LogManager::getSingleton().logMessage("RoomPortalWave name=" + getName()
            + "spawns a creature class=" + classToSpawn->getClassName()
            + ", name=" + newCreature->getName() + ", seatId=" + Helper::toString(getSeat()->getId()));

        newCreature->addToGameMap();
        newCreature->createMesh();
        newCreature->setPosition(newCreature->getPosition(), false);

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

    os << mClaimedValue << "\t" << mTurnsBetween2Waves << "\n";
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

bool RoomPortalWave::handleSearchFoe()
{
    if(mPortalObject == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "room=" + getName());
        return false;
    }

    Tile* tileStart = mPortalObject->getPositionTile();
    if(tileStart == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "room=" + getName());
        return false;
    }

    std::vector<Creature*> creatures = getGameMap()->getCreaturesBySeat(getSeat());
    if(creatures.empty())
        return false;

    // We randomly pick a creature to test for path
    uint32_t index = Random::Uint(0, creatures.size() - 1);
    Creature* creature = creatures[index];

    std::vector<Room*> dungeonTemples = getGameMap()->getRoomsByType(RoomType::dungeonTemple);

    // First, we check if a dungeon temple is reachable by ground. If yes, we cast a call to war
    // if not, we search the closest and try to go there
    std::vector<std::pair<Tile*,Ogre::Real>> tileDungeons;
    for(Room* room : dungeonTemples)
    {
        if(room->getSeat()->isAlliedSeat(getSeat()))
            continue;

        Tile* tile = room->getCentralTile();
        if(tile == nullptr)
            continue;

        if(!getGameMap()->pathExists(creature, tileStart, tile))
            continue;

        auto it = tileDungeons.begin();
        Ogre::Real templeDist = Pathfinding::squaredDistanceTile(*tileStart, *tile);
        while(it != tileDungeons.end())
        {
            if(templeDist < it->second)
                break;

            ++it;
        }
        tileDungeons.insert(it, std::pair<Tile*,Ogre::Real>(tile, templeDist));
    }

    // If we cannot reach a dungeon temple. We need to dig.
    if(tileDungeons.empty())
        return false;

    // We found an accessible tile. Let's go there
    // If there is already a call to war, we don't cast another one
    for(Spell* spell : getGameMap()->getSpells())
    {
        if(spell->getSpellType() != SpellType::callToWar)
            continue;

        // There is a call to war. We do not cast another one but there is no need to dig
        return true;
    }

    // No call to war. We cast it at the closest dungeon (they are sorted in tileDungeons)
    Tile* tile = tileDungeons[0].first;
    SpellCallToWar* spell = new SpellCallToWar(getGameMap());
    spell->setSeat(getSeat());
    spell->addToGameMap();
    Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(tile->getX()),
                                static_cast<Ogre::Real>(tile->getY()),
                                static_cast<Ogre::Real>(0.0));
    spell->createMesh();
    spell->setPosition(spawnPosition, false);
    return true;
}

bool RoomPortalWave::handleDigging()
{
    // No need to try digging if no worker
    Creature* creature = getGameMap()->getWorkerForPathFinding(getSeat());
    if(creature == nullptr)
        return false;

    if(mPortalObject == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "room=" + getName());
        return false;
    }

    Tile* tileStart = mPortalObject->getPositionTile();
    if(tileStart == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "room=" + getName());
        return false;
    }

    // We check that the dungeon is still alive.
    if(!mTargetDungeon.empty())
    {
        Room* room = getGameMap()->getRoomByName(mTargetDungeon);
        if((room == nullptr) ||
           (room->numCoveredTiles() == 0))
        {
            mTargetDungeon.clear();
        }
    }

    // We check if our current path is valid
    bool isPathValid = true;
    std::vector<Tile*> tilesToMark;
    uint32_t nbTilesToDig = 0;
    for(Tile* tile : mMarkedTilesToEnemy)
    {
        if(creature->canGoThroughTile(tile))
            continue;

        if(tile->getMarkedForDigging(getSeat()->getPlayer()))
        {
            ++nbTilesToDig;
            continue;
        }

        if(tile->isDiggable(getSeat()))
        {
            ++nbTilesToDig;
            tilesToMark.push_back(tile);
            continue;
        }

        // We hit a non diggable tile. we need to find another way
        isPathValid = false;
        break;
    }

    if(isPathValid &&
       (nbTilesToDig > 0) &&
       !mTargetDungeon.empty())
    {
        // The currently marked tile path is valid. No need to change
        // we check if we already killed the given dungeon. If no, nothing to do. If yes, we go somewhere else
        getSeat()->getPlayer()->markTilesForDigging(true, tilesToMark, false);
        return true;
    }

    // We sort the dungeon temples by distance. We will try to reach the closest accessible one
    std::vector<std::pair<Tile*,Ogre::Real>> tileDungeons;
    std::vector<Room*> dungeonTemples = getGameMap()->getRoomsByType(RoomType::dungeonTemple);
    for(Room* room : dungeonTemples)
    {
        if(room->getSeat()->isAlliedSeat(getSeat()))
            continue;

        Tile* tile = room->getCentralTile();
        if(tile == nullptr)
            continue;

        auto it = tileDungeons.begin();
        Ogre::Real templeDist = Pathfinding::squaredDistanceTile(*tileStart, *tile);
        while(it != tileDungeons.end())
        {
            if(templeDist < it->second)
                break;

            ++it;
        }
        tileDungeons.insert(it, std::pair<Tile*,Ogre::Real>(tile, templeDist));
    }

    if(tileDungeons.empty())
    {
        // No reachable enemy dungeon
        return false;
    }

    bool isWayFound = false;
    tilesToMark.clear();
    for(std::pair<Tile*,Ogre::Real>& p : tileDungeons)
    {
        mMarkedTilesToEnemy.clear();
        if(!findBestDiggablePath(tileStart, p.first, creature, mMarkedTilesToEnemy) &&
           mMarkedTilesToEnemy.empty())
        {
            continue;
        }

        mTargetDungeon = p.first->getCoveringBuilding()->getName();
        isWayFound = true;
        for(Tile* tile : mMarkedTilesToEnemy)
            tilesToMark.push_back(tile);

        break;
    }

    if(!isWayFound)
        return false;

    getSeat()->getPlayer()->markTilesForDigging(true, tilesToMark, false);

    return true;
}

bool RoomPortalWave::findBestDiggablePath(Tile* tileStart, Tile* tileDest, Creature* creature, std::vector<Tile*>& tiles)
{
    if(!creature->canGoThroughTile(tileStart) &&
       !tileStart->isDiggable(creature->getSeat()))
    {
        return false;
    }

    std::vector<std::pair<int, int>> rotations;
    std::vector<TileSearch> blockingTiles;
    Tile* tile = tileStart;
    Tile* lastTileBlocked = nullptr;
    bool currentRotationClockWise = false;
    while(true)
    {
        if(tile == tileDest)
            return true;

        if(tile == nullptr)
        {
            // We reach the end of the map.
            // We try the last blocking tile we found in the other direction (if any)
            lastTileBlocked = nullptr;
            for(auto it = blockingTiles.rbegin(); it != blockingTiles.rend(); ++it)
            {
                TileSearch& tileSearch = *it;
                if(tileSearch.mRotationClockWise &&
                   tileSearch.mRotationCounterClockWise)
                {
                    continue;
                }

                lastTileBlocked = tileSearch.mTile;
                if(tileSearch.mRotationClockWise)
                {
                    tileSearch.mRotationCounterClockWise = true;
                    currentRotationClockWise = false;
                }
                else
                {
                    tileSearch.mRotationClockWise = true;
                    currentRotationClockWise = true;
                }

                rotations.clear();
                rotations.push_back(tileSearch.mDirectionHit);
                rotations.push_back(TileSearch::computeNextRotation(rotations.back(), currentRotationClockWise));
                break;
            }

            if(lastTileBlocked == nullptr)
                return false;

            tile = getGameMap()->getTile(lastTileBlocked->getX() + rotations.back().first, lastTileBlocked->getY() + rotations.back().second);
        }

        // A nullptr tile is possible if we are searching near the map border after applying the rotation
        if(tile == nullptr)
            continue;

        bool isTilePassable = false;
        // If the tile is walkable, no need to dig it
        if(creature->canGoThroughTile(tile))
        {
            isTilePassable = true;
        }
        else if(tile->getMarkedForDigging(creature->getSeat()->getPlayer()))
        {
            isTilePassable = true;
        }
        else if(tile->isDiggable(creature->getSeat()))
        {
            tiles.push_back(tile);
            isTilePassable = true;
        }

        if(isTilePassable)
        {
            // The tile is passable. If we were rotating, we try the next rotation
            // If not, we continue our way
            if(!rotations.empty())
                rotations.pop_back();

            if(rotations.empty())
            {
                int diffX = tileDest->getX() - tile->getX();
                int diffY = tileDest->getY() - tile->getY();
                if(std::abs(diffX) > std::abs(diffY))
                {
                    if(diffX < 0)
                        rotations.push_back(std::pair<int, int>(-1, 0));
                    else
                        rotations.push_back(std::pair<int, int>(1, 0));
                }
                else
                {
                    if(diffY < 0)
                        rotations.push_back(std::pair<int, int>(0, -1));
                    else
                        rotations.push_back(std::pair<int, int>(0, 1));
                }

                // We are not blocked
                lastTileBlocked = nullptr;
            }
            tile = getGameMap()->getTile(tile->getX() + rotations.back().first, tile->getY() + rotations.back().second);
        }
        else
        {
            // We start from the previous tile (that was good to go)
            tile = getGameMap()->getTile(tile->getX() - rotations.back().first, tile->getY() - rotations.back().second);
            if(tile == nullptr)
            {
                OD_ASSERT_TRUE_MSG(false, "room=" + getName());
                return false;
            }

            // Tile is not passable. We try to go around. If we are not already following a wall, we start to
            if(lastTileBlocked == nullptr)
            {
                // We check if we already hit this tile
                bool isFound = false;
                for(TileSearch& tileSearch : blockingTiles)
                {
                    if(tileSearch.mTile != tile)
                        continue;

                    // We already hit this tile. We stop searching this way
                    isFound = true;
                }

                if(isFound)
                {
                    tile = nullptr;
                    continue;
                }

                // We have never been blocked here
                lastTileBlocked = tile;

                int diffX = tileDest->getX() - tile->getX();
                int diffY = tileDest->getY() - tile->getY();
                if(rotations.back().first == 0)
                {
                    if(diffX < 0)
                        currentRotationClockWise = (rotations.back().second > 0);
                    else
                        currentRotationClockWise = (rotations.back().second < 0);
                }
                else // rotations.back().second should be 0
                {
                    if(diffY < 0)
                        currentRotationClockWise = (rotations.back().first < 0);
                    else
                        currentRotationClockWise = (rotations.back().first > 0);
                }
                TileSearch tileSearch;
                tileSearch.mTile = tile;
                tileSearch.mDirectionHit = rotations.back();
                if(currentRotationClockWise)
                    tileSearch.mRotationClockWise = true;
                else
                    tileSearch.mRotationCounterClockWise = true;

                blockingTiles.push_back(tileSearch);
            }

            // We set next direction
            rotations.push_back(TileSearch::computeNextRotation(rotations.back(), currentRotationClockWise));
            tile = getGameMap()->getTile(tile->getX() + rotations.back().first, tile->getY() + rotations.back().second);
        }
    }
}
