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
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "rooms/RoomManager.h"
#include "spells/SpellCallToWar.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

#include <vector>

static RoomManagerRegister<RoomPortalWave> reg(RoomType::portalWave, "PortalWave", "Wave portal room");

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
        mClaimedValue(0),
        mTargetDungeon(nullptr),
        mIsFirstUpkeep(true),
        mStrategy(RoomPortalWaveStrategy::closestDungeon),
        mRangeTilesAttack(-1)
{
   setMeshName("");
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
    return !getSeat()->isAlliedSeat(seat);
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
    // Delete all previous rooms meshes and recreate a central one.
    removeAllBuildingObjects();
    mPortalObject = nullptr;

    Tile* centralTile = getCentralTile();
    if (centralTile == nullptr)
        return;

    mPortalObject = new PersistentObject(getGameMap(), true, getName(), "KnightCoffin", centralTile, 0.0, false);
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

    if(mIsFirstUpkeep)
    {
        mIsFirstUpkeep = false;
        handleFirstUpkeep();
        handleChooseTarget();
    }

    if(mSearchFoeCountdown > 0)
        --mSearchFoeCountdown;
    else
    {
        mSearchFoeCountdown = Random::Uint(10, 20);

        handleAttack();
    }

    if (mSpawnCountdown < mTurnsBetween2Waves)
    {
        ++mSpawnCountdown;
        mPortalObject->setAnimationState("Idle");
        return;
    }

    // At each new wave, we try to choose a target
    handleChooseTarget();

    // Spawns a new wave
    mSpawnCountdown = 0;

    handleSpawnWave();
}

void RoomPortalWave::handleChooseTarget()
{
    // We choose an enemy depending on the strategy
    if(!mAttackableSeats.empty())
    {
        switch(mStrategy)
        {
            case RoomPortalWaveStrategy::randomPlayer:
            {
                uint32_t kk = Random::Uint(0, mAttackableSeats.size() - 1);
                mTargetSeats.clear();
                Seat* attackedSeat = mAttackableSeats[kk];
                OD_LOG_INF("PortalWave=" + getName() + ", attacking seatId=" + Helper::toString(attackedSeat->getId()));
                mTargetSeats.push_back(attackedSeat);
                if(mTargetDungeon != nullptr)
                    mTargetDungeon->removeGameEntityListener(this);

                mTargetDungeon = nullptr;
                break;
            }
            case RoomPortalWaveStrategy::fixedTeamIds:
            case RoomPortalWaveStrategy::closestDungeon:
            default:
                mTargetSeats = mAttackableSeats;
                break;
        }
    }
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
            OD_LOG_ERR("RoomPortalWave=" + getName() + ", wrong creature class=" + p.first);
            continue;
        }

        Creature* newCreature = new Creature(getGameMap(), true, classToSpawn, getSeat(), spawnPosition);
        newCreature->setLevel(p.second);
        newCreature->setHP(newCreature->getMaxHp());

        OD_LOG_INF("RoomPortalWave name=" + getName()
            + "spawns a creature class=" + classToSpawn->getClassName()
            + ", name=" + newCreature->getName() + ", seatId=" + Helper::toString(getSeat()->getId()));

        newCreature->addToGameMap();
        newCreature->createMesh();
        newCreature->setPosition(newCreature->getPosition());

        --maxCreaturesToSpawn;
    }
}

void RoomPortalWave::handleAttack()
{
    // We check if new foe has claimed a tile in our border
    if(mRangeTilesAttack > 0)
    {
        for(Tile* tile : mTilesBorder)
        {
            if(!tile->isClaimed())
                continue;

            Seat* tileSeat = tile->getSeat();
            if(tileSeat == nullptr)
            {
                OD_LOG_ERR("nullptr seat claimed tile=" + Tile::displayAsString(tile));
                continue;
            }

            if(std::find(mAttackableSeats.begin(), mAttackableSeats.end(), tileSeat) != mAttackableSeats.end())
                continue;

            if(getSeat()->isAlliedSeat(tileSeat))
                continue;

            if((mStrategy == RoomPortalWaveStrategy::fixedTeamIds) &&
               (std::find(mTargetTeams.begin(), mTargetTeams.end(), tileSeat->getTeamId()) == mTargetTeams.end()))
            {
                continue;
            }

            mAttackableSeats.push_back(tileSeat);
            // Tells the player he is going to be attacked
            if(tileSeat->getPlayer() != nullptr && tileSeat->getPlayer()->getIsHuman())
            {
                ServerNotification *serverNotification = new ServerNotification(
                    ServerNotificationType::chatServer, tileSeat->getPlayer());

                std::string msg = "Your evil presence has soiled this holy land for too long. You shall be crushed by our blessed swords !";
                serverNotification->mPacket << msg << EventShortNoticeType::majorGameEvent;
                ODServer::getSingleton().queueServerNotification(serverNotification);
            }
        }
    }

    if(!mAttackableSeats.empty())
    {
        if(!handleSearchFoe())
            handleDigging();
    }
}

void RoomPortalWave::handleSpawnWave()
{
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

void RoomPortalWave::setupRoom(const std::string& name, Seat* seat, const std::vector<Tile*>& tiles)
{
    Room::setupRoom(name, seat, tiles);
    mClaimedValue = static_cast<double>(numCoveredTiles());
}

void RoomPortalWave::exportToStream(std::ostream& os) const
{
    Room::exportToStream(os);

    std::string teams;
    if(mTargetTeams.empty())
    {
        teams = "-";
    }
    else
    {
        for(int team : mTargetTeams)
        {
            if(teams.empty())
                teams = Helper::toString(team);
            else
                teams += "/" + Helper::toString(team);
        }
    }

    os << mClaimedValue << "\t" << mTurnsBetween2Waves << "\t" << mStrategy << "\t" << mRangeTilesAttack << "\t" << teams << "\n";
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
    OD_ASSERT_TRUE(is >> mStrategy);
    OD_ASSERT_TRUE(is >> mRangeTilesAttack);
    std::string teamsStr;
    OD_ASSERT_TRUE(is >> teamsStr);
    mTargetTeams.clear();
    if(teamsStr != "-")
    {
        std::vector<std::string> teams = Helper::split(teamsStr, '/', true);
        for(const std::string& teamStr : teams)
        {
            int teamId = Helper::toInt(teamStr);
            if(teamId == 0)
            {
                OD_LOG_ERR("Cannot use team 0 as a target for portal wave=" + getName());
                continue;
            }
            mTargetTeams.push_back(teamId);
        }
    }

    std::string str;
    OD_ASSERT_TRUE(is >> str);
    if(str != "[Waves]")
    {
        OD_LOG_ERR("RoomPortalWave=" + getName() + ", str=" + str);
        return;
    }
    while(true)
    {
        OD_ASSERT_TRUE(is >> str);
        if(str == "[/Waves]")
            break;

        if(str != "[Wave]")
        {
            OD_LOG_ERR("RoomPortalWave=" + getName() + ", str=" + str);
            return;
        }

        RoomPortalWaveData* roomPortalWaveData = new RoomPortalWaveData;
        OD_ASSERT_TRUE(is >> str);
        if(str != "SpawnTurnMin")
        {
            OD_LOG_ERR("RoomPortalWave=" + getName() + ", str=" + str);
            delete roomPortalWaveData;
            return;
        }
        OD_ASSERT_TRUE(is >> roomPortalWaveData->mSpawnTurnMin);

        OD_ASSERT_TRUE(is >> str);
        if(str != "SpawnTurnMax")
        {
            OD_LOG_ERR("RoomPortalWave=" + getName() + ", str=" + str);
            delete roomPortalWaveData;
            return;
        }
        OD_ASSERT_TRUE(is >> roomPortalWaveData->mSpawnTurnMax);

        OD_ASSERT_TRUE(is >> str);
        if(str != "[Creatures]")
        {
            OD_LOG_ERR("RoomPortalWave=" + getName() + ", str=" + str);
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
            OD_LOG_ERR("RoomPortalWave=" + getName() + ", str=" + str);
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
        OD_LOG_ERR("RoomPortalWave=" + getName());
        return;
    }

    Tile* tilePortalObject = mPortalObject->getPositionTile();
    if(tilePortalObject == nullptr)
    {
        OD_LOG_ERR("RoomPortalWave=" + getName() + ", mPortalObject=" + mPortalObject->getName());
        return;
    }
    TileData* tileData = mTileData[tilePortalObject];
    if(tileData == nullptr)
    {
        OD_LOG_ERR("RoomPortalWave=" + getName() + ", tile=" + Tile::displayAsString(tilePortalObject));
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
        OD_LOG_ERR("room=" + getName());
        return false;
    }

    Tile* tileStart = mPortalObject->getPositionTile();
    if(tileStart == nullptr)
    {
        OD_LOG_ERR("room=" + getName());
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
        // We check if the strategy allows us to attack the dungeon
        if(std::find(mTargetSeats.begin(), mTargetSeats.end(), room->getSeat()) == mTargetSeats.end())
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
    SpellCallToWar* spell = new SpellCallToWar(getGameMap(), true);
    spell->setSeat(getSeat());
    spell->addToGameMap();
    Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(tile->getX()),
                                static_cast<Ogre::Real>(tile->getY()),
                                static_cast<Ogre::Real>(0.0));
    spell->createMesh();
    spell->setPosition(spawnPosition);
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
        OD_LOG_ERR("room=" + getName());
        return false;
    }

    Tile* tileStart = mPortalObject->getPositionTile();
    if(tileStart == nullptr)
    {
        OD_LOG_ERR("room=" + getName());
        return false;
    }

    // We check that the dungeon is still alive.
    if(mTargetDungeon != nullptr)
    {
        if(mTargetDungeon->numCoveredTiles() == 0)
        {
            mTargetDungeon->removeGameEntityListener(this);
            mTargetDungeon = nullptr;
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
       (mTargetDungeon != nullptr))
    {
        // The currently marked tile path is valid. No need to change
        // we check if we already killed the given dungeon. If no, nothing to do. If yes, we go somewhere else
        getSeat()->getPlayer()->markTilesForDigging(true, tilesToMark, false);
        return true;
    }

    // We sort the dungeon temples by distance. We will try to reach the closest accessible one
    std::vector<std::pair<Room*,Ogre::Real>> tileDungeons;
    std::vector<Room*> dungeonTemples = getGameMap()->getRoomsByType(RoomType::dungeonTemple);
    for(Room* room : dungeonTemples)
    {
        // We check if the strategy allows us to attack the dungeon
        if(std::find(mTargetSeats.begin(), mTargetSeats.end(), room->getSeat()) == mTargetSeats.end())
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
        tileDungeons.insert(it, std::pair<Room*,Ogre::Real>(room, templeDist));
    }

    if(tileDungeons.empty())
    {
        // No reachable enemy dungeon
        return false;
    }

    bool isWayFound = false;
    tilesToMark.clear();
    for(std::pair<Room*,Ogre::Real>& p : tileDungeons)
    {
        mMarkedTilesToEnemy.clear();
        Tile* tileDungeon = p.first->getCentralTile();
        if(tileDungeon == nullptr)
            continue;

        if(!findBestDiggablePath(tileStart, tileDungeon, creature, mMarkedTilesToEnemy) &&
           mMarkedTilesToEnemy.empty())
        {
            continue;
        }

        if(mTargetDungeon != nullptr)
            mTargetDungeon->removeGameEntityListener(this);

        mTargetDungeon = p.first;
        mTargetDungeon->addGameEntityListener(this);
        OD_LOG_INF("PortalWave=" + getName()+ " wants to attack dungeon=" + mTargetDungeon->getName());
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
                OD_LOG_ERR("room=" + getName());
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

void RoomPortalWave::handleFirstUpkeep()
{
    // We check the dungeons is in range
    Tile* myTile = getCentralTile();
    if(myTile == nullptr)
    {
        OD_LOG_ERR("nullptr tile room=" + getName());
        return;
    }

    if(mRangeTilesAttack <= 0)
    {
        // We take all dungeons
        std::vector<Room*> dungeonTemples = getGameMap()->getRoomsByType(RoomType::dungeonTemple);
        for(Room* room : dungeonTemples)
        {
            Seat* roomSeat = room->getSeat();
            if(roomSeat->isAlliedSeat(getSeat()))
                continue;

            if((mStrategy == RoomPortalWaveStrategy::fixedTeamIds) &&
               (std::find(mTargetTeams.begin(), mTargetTeams.end(), roomSeat->getTeamId()) == mTargetTeams.end()))
            {
                continue;
            }

            if(std::find(mAttackableSeats.begin(), mAttackableSeats.end(), roomSeat) == mAttackableSeats.end())
            {
                mAttackableSeats.push_back(roomSeat);
            }
        }
        return;
    }

    double squaredRange = static_cast<double>(mRangeTilesAttack) * static_cast<double>(mRangeTilesAttack);
    std::vector<Room*> dungeonTemples = getGameMap()->getRoomsByType(RoomType::dungeonTemple);
    for(Room* room : dungeonTemples)
    {
        Seat* roomSeat = room->getSeat();
        if(roomSeat->isAlliedSeat(getSeat()))
            continue;

        if((mStrategy == RoomPortalWaveStrategy::fixedTeamIds) &&
           (std::find(mTargetTeams.begin(), mTargetTeams.end(), roomSeat->getTeamId()) == mTargetTeams.end()))
        {
            continue;
        }

        Tile* centralTile = room->getCentralTile();
        double dist = Pathfinding::squaredDistanceTile(*centralTile, *myTile);
        if(dist > squaredRange)
            continue;

        if(std::find(mAttackableSeats.begin(), mAttackableSeats.end(), roomSeat) == mAttackableSeats.end())
        {
            mAttackableSeats.push_back(roomSeat);
        }
    }

    // We only keep the tiles on the border. That means that if a player starts with claimed
    // tiles in range of the portal (but not its dungeon heart), it will not get attacked until
    // he starts claiming tiles on the portal's border. That is acceptable and will avoid to
    // check all the tiles in range to focus on the border only (which will cover 99% of the
    // real cases).
    int32_t index = 0;
    while(true)
    {
        int32_t x = index;
        double yDouble = std::sqrt(squaredRange - static_cast<double>(x * x));
        int32_t y = Helper::round(yDouble);
        // If x > y, we should have read all tiles
        if(x > y)
            break;

        Tile* tile;
        // Tile north-west
        tile = getGameMap()->getTile(myTile->getX() + x, myTile->getY() + y);
        if(tile != nullptr)
            mTilesBorder.push_back(tile);
        // Tile north-east
        tile = getGameMap()->getTile(myTile->getX() - x, myTile->getY() + y);
        if((x != 0) && (tile != nullptr))
            mTilesBorder.push_back(tile);
        // Tile south-west
        tile = getGameMap()->getTile(myTile->getX() + x, myTile->getY() - y);
        if(tile != nullptr)
            mTilesBorder.push_back(tile);
        // Tile south-east
        tile = getGameMap()->getTile(myTile->getX() - x, myTile->getY() - y);
        if((x != 0) && (tile != nullptr))
            mTilesBorder.push_back(tile);
        // Tile east-north
        tile = getGameMap()->getTile(myTile->getX() - y, myTile->getY() + x);
        if((x != y) && (tile != nullptr))
            mTilesBorder.push_back(tile);
        // Tile east-south
        tile = getGameMap()->getTile(myTile->getX() - y, myTile->getY() - x);
        if((x != 0) && (x != y) && (tile != nullptr))
            mTilesBorder.push_back(tile);
        // Tile west-north
        tile = getGameMap()->getTile(myTile->getX() + y, myTile->getY() + x);
        if((x != y) && (tile != nullptr))
            mTilesBorder.push_back(tile);
        // Tile west-south
        tile = getGameMap()->getTile(myTile->getX() + y, myTile->getY() - x);
        if((x != 0) && (x != y) && (tile != nullptr))
            mTilesBorder.push_back(tile);

        ++index;
    }
}

bool RoomPortalWave::notifyRemovedFromGameMap(GameEntity* entity)
{
    if(entity->getObjectType() == GameEntityType::room)
    {
        Room* room = static_cast<Room*>(entity);
        if(mTargetDungeon == room)
            mTargetDungeon = nullptr;

        return false;
    }

    return true;
}

void RoomPortalWave::checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    // Not buildable on game mode
}

bool RoomPortalWave::buildRoom(GameMap* gameMap, Player* player, ODPacket& packet)
{
    // Not buildable on game mode
    return false;
}

void RoomPortalWave::checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    checkBuildRoomDefaultEditor(gameMap, RoomType::portalWave, inputManager, inputCommand);
}

bool RoomPortalWave::buildRoomEditor(GameMap* gameMap, ODPacket& packet)
{
    RoomPortalWave* room = new RoomPortalWave(gameMap);
    return buildRoomDefaultEditor(gameMap, room, packet);
}

Room* RoomPortalWave::getRoomFromStream(GameMap* gameMap, std::istream& is)
{
    RoomPortalWave* room = new RoomPortalWave(gameMap);
    room->importFromStream(is);
    return room;
}

std::ostream& operator<<(std::ostream& os, const RoomPortalWaveStrategy& type)
{
    os << static_cast<int32_t>(type);
    return os;
}

std::istream& operator>>(std::istream& is, RoomPortalWaveStrategy& type)
{
    int32_t tmp;
    OD_ASSERT_TRUE(is >> tmp);
    type = static_cast<RoomPortalWaveStrategy>(tmp);
    return is;
}
