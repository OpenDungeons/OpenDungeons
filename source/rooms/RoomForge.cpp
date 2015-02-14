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

#include "rooms/RoomForge.h"

#include "entities/CraftedTrap.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/Tile.h"

#include "gamemap/GameMap.h"

#include "traps/Trap.h"

#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

const Ogre::Real OFFSET_CREATURE = 0.3;
const Ogre::Real OFFSET_SPOT = 0.3;

RoomForge::RoomForge(GameMap* gameMap) :
    Room(gameMap),
    mNbTurnsNoChangeSpots(0),
    mPoints(0),
    mTrapType(TrapType::nullTrapType)
{
    setMeshName("Forge");
}

RenderedMovableEntity* RoomForge::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    switch(place)
    {
        case ActiveSpotPlace::activeSpotCenter:
        {
            Ogre::Real x = static_cast<Ogre::Real>(tile->getX());
            Ogre::Real y = static_cast<Ogre::Real>(tile->getY());
            y += OFFSET_SPOT;
            std::vector<Tile*>::iterator it = std::find(mAllowedSpotsForCraftedItems.begin(), mAllowedSpotsForCraftedItems.end(), tile);
            if(it != mAllowedSpotsForCraftedItems.end())
                mAllowedSpotsForCraftedItems.erase(it);

            mUnusedSpots.push_back(tile);
            return loadBuildingObject(getGameMap(), "Foundry", tile, x, y, 45.0, false);
        }
        case ActiveSpotPlace::activeSpotLeft:
        {
            return loadBuildingObject(getGameMap(), "Chimney", tile, 90.0, false);
        }
        case ActiveSpotPlace::activeSpotRight:
        {
            return loadBuildingObject(getGameMap(), "Chimney", tile, 270.0, false);
        }
        case ActiveSpotPlace::activeSpotTop:
        {
            return loadBuildingObject(getGameMap(), "Grindstone", tile, 180.0, false);
        }
        case ActiveSpotPlace::activeSpotBottom:
        {
            return loadBuildingObject(getGameMap(), "Anvil", tile, 0.0, false);
        }
    }
    return nullptr;
}

void RoomForge::absorbRoom(Room *r)
{
    OD_ASSERT_TRUE_MSG(r->getType() == getType(), "Trying to merge incompatible rooms: " + getName()
        + ", with " + r->getName());
    RoomForge* roomAbs = static_cast<RoomForge*>(r);
    mUnusedSpots.insert(mUnusedSpots.end(), roomAbs->mUnusedSpots.begin(), roomAbs->mUnusedSpots.end());
    roomAbs->mUnusedSpots.clear();
    mAllowedSpotsForCraftedItems.insert(mAllowedSpotsForCraftedItems.end(), roomAbs->mAllowedSpotsForCraftedItems.begin(), roomAbs->mAllowedSpotsForCraftedItems.end());
    roomAbs->mAllowedSpotsForCraftedItems.clear();

    mPoints += roomAbs->mPoints;
    roomAbs->mPoints = 0;

    Room::absorbRoom(r);
}

void RoomForge::addCoveredTile(Tile* t, double nHP)
{
    mAllowedSpotsForCraftedItems.push_back(t);

    Room::addCoveredTile(t, nHP);
}

bool RoomForge::removeCoveredTile(Tile* t)
{
    std::vector<Tile*>::iterator it = std::find(mAllowedSpotsForCraftedItems.begin(), mAllowedSpotsForCraftedItems.end(), t);
    if(it != mAllowedSpotsForCraftedItems.end())
        mAllowedSpotsForCraftedItems.erase(it);

    return Room::removeCoveredTile(t);
}

void RoomForge::notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
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

bool RoomForge::hasOpenCreatureSpot(Creature* c)
{
    // If there is no need, we do not allow creature to work
    if(mTrapType == TrapType::nullTrapType)
        return false;

    // We accept all creatures as soon as there are free active spots
    // We start by counting the crafted objects
    uint32_t nbCraftedItems = countCraftedItemsOnRoom();
    if(nbCraftedItems >= (getNumActiveSpots() - mCreaturesSpots.size()))
        return false;

    return !mUnusedSpots.empty();
}

bool RoomForge::addCreatureUsingRoom(Creature* creature)
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

void RoomForge::removeCreatureUsingRoom(Creature* c)
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

void RoomForge::doUpkeep()
{
    Room::doUpkeep();

    if (mCoveredTiles.empty())
        return;

    // If we are not already working on something, we check if a trap have a need. If so,
    // we check that no reachable forge can supply the trap before starting crafting
    if(mTrapType == TrapType::nullTrapType)
    {
        std::map<TrapType, int> neededTraps;
        Creature* worker = getGameMap()->getWorkerForPathFinding(getSeat());
        if (worker != nullptr)
        {
            std::vector<Building*> reachableBuildings = getGameMap()->getReachableBuildingsPerSeat(getSeat(),
                mCoveredTiles[0], worker);
            for(Building* building : reachableBuildings)
            {
                if(building->getObjectType() != GameEntityType::trap)
                    continue;

                Trap* trap = static_cast<Trap*>(building);

                int32_t nbNeededCraftedTrap = trap->getNbNeededCraftedTrap();
                if(nbNeededCraftedTrap <= 0)
                    continue;

                neededTraps[trap->getType()] += nbNeededCraftedTrap;
            }
        }

        if(!neededTraps.empty())
        {
            // We check if there are enough owned reachable crafted traps
            const std::vector<RenderedMovableEntity*>& renderables = getGameMap()->getRenderedMovableEntities();
            for(std::pair<TrapType const, int>& p : neededTraps)
            {
                for(RenderedMovableEntity* renderable : renderables)
                {
                    if(renderable->getObjectType() != GameEntityType::craftedTrap)
                        continue;

                    if(renderable->getSeat() != getSeat())
                        continue;

                    // If the crafted trap is being carried, it should not be counted because
                    // the trap it is carried to should have booked a spot
                    if(!renderable->getIsOnMap())
                        continue;

                    CraftedTrap* craftedTrap = static_cast<CraftedTrap*>(renderable);
                    if(craftedTrap->getTrapType() != p.first)
                        continue;

                    --p.second;
                }
            }

            std::vector<TrapType> trapsToCraft;
            for(std::pair<TrapType const, int>& p : neededTraps)
            {
                if(p.second <= 0)
                    continue;

                trapsToCraft.push_back(p.first);
            }

            // We randomly pickup the trap to craft if any
            if(!trapsToCraft.empty())
            {
                uint32_t index = Random::Uint(0, trapsToCraft.size() - 1);
                mTrapType = trapsToCraft[index];
            }
        }
    }

    // If there is nothing to do, we remove the working creatures if any
    if(mTrapType == TrapType::nullTrapType)
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

                mPoints += static_cast<int32_t>(creatureRoomAffinity.getEfficiency() * ConfigManager::getSingleton().getRoomConfigDouble("ForgePointsPerWork"));
                creature->jobDone(ConfigManager::getSingleton().getRoomConfigDouble("ForgeAwaknessPerWork"));
                creature->setJobCooldown(Random::Uint(ConfigManager::getSingleton().getRoomConfigUInt32("ForgeCooldownWorkMin"),
                    ConfigManager::getSingleton().getRoomConfigUInt32("ForgeCooldownWorkMax")));
            }
        }
    }

    uint32_t nbCraftedItems = countCraftedItemsOnRoom();
    if(nbCraftedItems > (getNumActiveSpots() - mCreaturesSpots.size()))
    {
        // There is no available space. We remove a creature working here if there is one.
        // If there is none, it means the forge is full
        if(mCreaturesSpots.size() > 0)
        {
            for(const std::pair<Creature* const,Tile*>& p : mCreaturesSpots)
            {
                Creature* creature = p.first;
                creature->stopJob();
                break;
            }
        }
        else
            return;
    }

    // We check if we have enough forge points for the currently crafted trap
    int32_t pointsNeeded = Trap::getNeededForgePointsPerTrap(mTrapType);
    if(mPoints < pointsNeeded)
        return;

    // We check if there is an empty tile to release the craftedTrap
    Tile* tileCraftedTrap = checkIfAvailableSpot(mAllowedSpotsForCraftedItems);
    OD_ASSERT_TRUE_MSG(tileCraftedTrap != nullptr, "room=" + getName());
    if(tileCraftedTrap == nullptr)
        return;

    CraftedTrap* craftedTrap = new CraftedTrap(getGameMap(), getName(), mTrapType);
    craftedTrap->setSeat(getSeat());
    craftedTrap->addToGameMap();
    Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(tileCraftedTrap->getX()), static_cast<Ogre::Real>(tileCraftedTrap->getY()), static_cast<Ogre::Real>(0.0));
    craftedTrap->createMesh();
    craftedTrap->setPosition(spawnPosition, false);
    mPoints -= pointsNeeded;
    mTrapType = TrapType::nullTrapType;
}

uint32_t RoomForge::countCraftedItemsOnRoom()
{
    std::vector<GameEntity*> carryable;
    for(Tile* t : mCoveredTiles)
    {
        t->fillWithCarryableEntities(carryable);
    }
    uint32_t nbCraftedTrap = 0;
    for(GameEntity* entity : carryable)
    {
        if(entity->getObjectType() != GameEntityType::craftedTrap)
            continue;

        ++nbCraftedTrap;
    }

    return nbCraftedTrap;
}

Tile* RoomForge::checkIfAvailableSpot(const std::vector<Tile*>& activeSpots)
{
    for(Tile* tile : activeSpots)
    {
        // If the tile contains no crafted trap, we can add a new one
        bool isFilled = false;
        std::vector<GameEntity*> entities;
        tile->fillWithCarryableEntities(entities);
        for(GameEntity* entity : entities)
        {
            if(entity->getObjectType() != GameEntityType::craftedTrap)
                continue;

            // There is one
            isFilled = true;
            break;
        }

        if(isFilled)
            continue;

        return tile;
    }

    return nullptr;
}

void RoomForge::getCreatureWantedPos(Creature* creature, Tile* tileSpot,
    Ogre::Real& wantedX, Ogre::Real& wantedY)
{
    wantedX = static_cast<Ogre::Real>(tileSpot->getX());
    wantedY = static_cast<Ogre::Real>(tileSpot->getY());
    wantedY -= OFFSET_CREATURE;
}
