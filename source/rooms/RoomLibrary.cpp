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

#include "rooms/RoomLibrary.h"

#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/ResearchEntity.h"
#include "entities/Tile.h"
#include "game/Research.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "rooms/RoomManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

static RoomManagerRegister<RoomLibrary> reg(RoomType::library, "Library");

const Ogre::Real OFFSET_CREATURE = 0.3;
const Ogre::Real OFFSET_SPOT = 0.3;

RoomLibrary::RoomLibrary(GameMap* gameMap) :
    Room(gameMap)
{
    setMeshName("Library");
}

RenderedMovableEntity* RoomLibrary::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    switch(place)
    {
        case ActiveSpotPlace::activeSpotCenter:
        {
            RoomLibraryTileData* roomLibraryTileData = static_cast<RoomLibraryTileData*>(mTileData[tile]);
            roomLibraryTileData->mCanHaveResearchEntity = false;

            Ogre::Real x = static_cast<Ogre::Real>(tile->getX());
            Ogre::Real y = static_cast<Ogre::Real>(tile->getY());
            y += OFFSET_SPOT;
            mUnusedSpots.push_back(tile);
            if (Random::Int(0, 100) > 50)
                return loadBuildingObject(getGameMap(), "Podium", tile, x, y, 45.0, false);
            else
                return loadBuildingObject(getGameMap(), "Bookcase", tile, x, y, 45.0, false);
        }
        case ActiveSpotPlace::activeSpotLeft:
        {
            return loadBuildingObject(getGameMap(), "Bookshelf", tile, 90.0, false);
        }
        case ActiveSpotPlace::activeSpotRight:
        {
            return loadBuildingObject(getGameMap(), "Bookshelf", tile, 270.0, false);
        }
        case ActiveSpotPlace::activeSpotTop:
        {
            return loadBuildingObject(getGameMap(), "Bookshelf", tile, 0.0, false);
        }
        case ActiveSpotPlace::activeSpotBottom:
        {
            return loadBuildingObject(getGameMap(), "Bookshelf", tile, 180.0, false);
        }
        default:
            break;
    }
    return nullptr;
}

void RoomLibrary::absorbRoom(Room *r)
{
    OD_ASSERT_TRUE_MSG(r->getType() == getType(), "Trying to merge incompatible rooms: " + getName()
        + ", with " + r->getName());
    RoomLibrary* roomAbs = static_cast<RoomLibrary*>(r);
    mUnusedSpots.insert(mUnusedSpots.end(), roomAbs->mUnusedSpots.begin(), roomAbs->mUnusedSpots.end());
    roomAbs->mUnusedSpots.clear();
    mCreaturesSpots.insert(roomAbs->mCreaturesSpots.begin(), roomAbs->mCreaturesSpots.end());
    roomAbs->mCreaturesSpots.clear();

    Room::absorbRoom(r);
}

void RoomLibrary::notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
{
    Room::notifyActiveSpotRemoved(place, tile);

    if(place != ActiveSpotPlace::activeSpotCenter)
        return;

    for(const std::pair<Creature* const,Tile*>& p : mCreaturesSpots)
    {
        Tile* tmpTile = p.second;
        if(tmpTile == tile)
        {
            Creature* creature = p.first;
            creature->stopJob();
            // stopJob should have released mCreaturesSpots[creature]. Now, we just need to release the unused spot
            break;
        }
    }

    std::vector<Tile*>::iterator itEr = std::find(mUnusedSpots.begin(), mUnusedSpots.end(), tile);
    OD_ASSERT_TRUE_MSG(itEr != mUnusedSpots.end(), "name=" + getName() + ", tile=" + Tile::displayAsString(tile));
    if(itEr != mUnusedSpots.end())
        mUnusedSpots.erase(itEr);
}

bool RoomLibrary::hasOpenCreatureSpot(Creature* c)
{
    // If there is no need, we do not allow creature to work
    if(getSeat()->getPlayer() == nullptr)
        return false;

    if(!getSeat()->isResearching())
        return false;

    // We accept all creatures as soon as there are free active spots
    uint32_t nbItems = countResearchItemsOnRoom();
    if(nbItems >= (getNumActiveSpots() - mCreaturesSpots.size()))
        return false;

    return !mUnusedSpots.empty();
}

bool RoomLibrary::addCreatureUsingRoom(Creature* creature)
{
    if(!Room::addCreatureUsingRoom(creature))
        return false;

    int index = Random::Int(0, mUnusedSpots.size() - 1);
    Tile* tileSpot = mUnusedSpots[index];
    mUnusedSpots.erase(mUnusedSpots.begin() + index);
    mCreaturesSpots[creature] = tileSpot;
    const Ogre::Vector3& creaturePosition = creature->getPosition();
    Ogre::Real wantedX = -1;
    Ogre::Real wantedY = -1;
    getCreatureWantedPos(creature, tileSpot, wantedX, wantedY);
    if(creaturePosition.x != wantedX ||
       creaturePosition.y != wantedY)
    {
        // We move to the good tile
        std::list<Tile*> pathToSpot = getGameMap()->path(creature, tileSpot);
        OD_ASSERT_TRUE(!pathToSpot.empty());
        if(pathToSpot.empty())
            return true;

        creature->setWalkPath(pathToSpot, 0, false);
        // We add the last step to take account of the offset
        creature->addDestination(wantedX, wantedY);
        creature->setAnimationState("Walk");
    }

    return true;
}

void RoomLibrary::removeCreatureUsingRoom(Creature* c)
{
    Room::removeCreatureUsingRoom(c);
    if(mCreaturesSpots.count(c) > 0)
    {
        Tile* tileSpot = mCreaturesSpots[c];
        OD_ASSERT_TRUE(tileSpot != nullptr);
        if(tileSpot == nullptr)
            return;
        mUnusedSpots.push_back(tileSpot);
        mCreaturesSpots.erase(c);
    }
}

void RoomLibrary::doUpkeep()
{
    Room::doUpkeep();

    if (mCoveredTiles.empty())
        return;

    if(getSeat()->getPlayer() == nullptr)
        return;

    // If there is nothing to do, we remove the working creatures if any
    if(!getSeat()->isResearching())
    {
        if(mCreaturesSpots.empty())
            return;

        // We remove the creatures working here
        std::vector<Creature*> creatures;
        for(const std::pair<Creature* const,Tile*>& p : mCreaturesSpots)
        {
            creatures.push_back(p.first);
        }

        for(Creature* creature : creatures)
        {
            creature->stopJob();
        }
        return;
    }

    for(const std::pair<Creature* const,Tile*>& p : mCreaturesSpots)
    {
        Creature* creature = p.first;
        Tile* tileSpot = p.second;
        Tile* tileCreature = creature->getPositionTile();
        if(tileCreature == nullptr)
            continue;

        Ogre::Real wantedX = -1;
        Ogre::Real wantedY = -1;
        getCreatureWantedPos(creature, tileSpot, wantedX, wantedY);

        RenderedMovableEntity* ro = getBuildingObjectFromTile(tileSpot);
        OD_ASSERT_TRUE(ro != nullptr);
        if(ro == nullptr)
            continue;
        // We consider that the creature is in the good place if it is in the expected tile and not moving
        Tile* expectedDest = getGameMap()->getTile(Helper::round(wantedX), Helper::round(wantedY));
        OD_ASSERT_TRUE_MSG(expectedDest != nullptr, "room=" + getName() + ", creature=" + creature->getName());
        if(expectedDest == nullptr)
            continue;
        if((tileCreature == expectedDest) &&
           !creature->isMoving())
        {
            if (creature->getJobCooldown() > 0)
            {
                creature->setAnimationState("Idle");
                creature->setJobCooldown(creature->getJobCooldown() - 1);
            }
            else
            {
                Ogre::Vector3 walkDirection(ro->getPosition().x - creature->getPosition().x, ro->getPosition().y - creature->getPosition().y, 0);
                walkDirection.normalise();
                creature->setAnimationState("Attack1", false, walkDirection);

                ro->setAnimationState("Triggered", false);

                const CreatureRoomAffinity& creatureRoomAffinity = creature->getDefinition()->getRoomAffinity(getType());
                OD_ASSERT_TRUE_MSG(creatureRoomAffinity.getRoomType() == getType(), "name=" + getName() + ", creature=" + creature->getName()
                    + ", creatureRoomAffinityType=" + Ogre::StringConverter::toString(static_cast<int>(creatureRoomAffinity.getRoomType())));

                int32_t pointsEarned = static_cast<int32_t>(creatureRoomAffinity.getEfficiency() * ConfigManager::getSingleton().getRoomConfigDouble("LibraryPointsPerWork"));
                creature->jobDone(ConfigManager::getSingleton().getRoomConfigDouble("LibraryAwaknessPerWork"));
                creature->setJobCooldown(Random::Uint(ConfigManager::getSingleton().getRoomConfigUInt32("LibraryCooldownWorkMin"),
                    ConfigManager::getSingleton().getRoomConfigUInt32("LibraryCooldownWorkMax")));

                // We check if we have enough points for the current research
                const Research* researchDone = getSeat()->addResearchPoints(pointsEarned);
                if(researchDone == nullptr)
                    continue;

                // We check if there is an empty tile to release the researchEntity
                Tile* spawnTile = checkIfAvailableSpot();
                if(spawnTile == nullptr)
                {
                    OD_ASSERT_TRUE_MSG(false, "room=" + getName());
                    return;
                }

                ResearchEntity* researchEntity = new ResearchEntity(getGameMap(), getName(), researchDone->getType());
                researchEntity->setSeat(getSeat());
                researchEntity->addToGameMap();
                Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(spawnTile->getX()), static_cast<Ogre::Real>(spawnTile->getY()), static_cast<Ogre::Real>(0.0));
                researchEntity->createMesh();
                researchEntity->setPosition(spawnPosition, false);

                // Tell the seat a research entity is waiting to be brought to the temple.
                getSeat()->addResearchWaiting(researchDone->getType());
            }
        }
    }

    uint32_t nbItems = countResearchItemsOnRoom();
    if(nbItems > (getNumActiveSpots() - mCreaturesSpots.size()))
    {
        // There is no available space. We remove a creature working here if there is one.
        // If there is none, it means the library is full
        if(mCreaturesSpots.empty())
            return;

        for(const std::pair<Creature* const,Tile*>& p : mCreaturesSpots)
        {
            Creature* creature = p.first;
            creature->stopJob();
            break;
        }
    }
}

uint32_t RoomLibrary::countResearchItemsOnRoom()
{
    std::vector<GameEntity*> carryable;
    for(Tile* t : mCoveredTiles)
    {
        t->fillWithCarryableEntities(carryable);
    }
    uint32_t nbItems = 0;
    for(GameEntity* entity : carryable)
    {
        if(entity->getObjectType() != GameEntityType::researchEntity)
            continue;

        ++nbItems;
    }

    return nbItems;
}

Tile* RoomLibrary::checkIfAvailableSpot()
{
    for(std::pair<Tile* const, TileData*>& p : mTileData)
    {
        RoomLibraryTileData* roomLibraryTileData = static_cast<RoomLibraryTileData*>(p.second);
        if(!roomLibraryTileData->mCanHaveResearchEntity)
            continue;

        // If the tile contains no item, we can add a new one
        bool isFilled = false;
        std::vector<GameEntity*> entities;
        p.first->fillWithCarryableEntities(entities);
        for(GameEntity* entity : entities)
        {
            if(entity->getObjectType() != GameEntityType::researchEntity)
                continue;

            // There is one
            isFilled = true;
            break;
        }

        if(isFilled)
            continue;

        return p.first;
    }

    return nullptr;
}

void RoomLibrary::getCreatureWantedPos(Creature* creature, Tile* tileSpot,
    Ogre::Real& wantedX, Ogre::Real& wantedY)
{
    wantedX = static_cast<Ogre::Real>(tileSpot->getX());
    wantedY = static_cast<Ogre::Real>(tileSpot->getY());
    wantedY -= OFFSET_CREATURE;
}

RoomLibraryTileData* RoomLibrary::createTileData(Tile* tile)
{
    return new RoomLibraryTileData;
}

int RoomLibrary::getRoomCost(std::vector<Tile*>& tiles, GameMap* gameMap, RoomType type,
    int tileX1, int tileY1, int tileX2, int tileY2, Player* player)
{
    return getRoomCostDefault(tiles, gameMap, type, tileX1, tileY1, tileX2, tileY2, player);
}

void RoomLibrary::buildRoom(GameMap* gameMap, const std::vector<Tile*>& tiles, Seat* seat)
{
    RoomLibrary* room = new RoomLibrary(gameMap);
    buildRoomDefault(gameMap, room, tiles, seat);
}

Room* RoomLibrary::getRoomFromStream(GameMap* gameMap, std::istream& is)
{
    return new RoomLibrary(gameMap);
}
