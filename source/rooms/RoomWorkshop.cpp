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

#include "rooms/RoomWorkshop.h"

#include "entities/CraftedTrap.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "gamemap/GameMap.h"
#include "rooms/RoomManager.h"
#include "traps/Trap.h"
#include "traps/TrapManager.h"
#include "traps/TrapType.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

static RoomManagerRegister<RoomWorkshop> reg(RoomType::workshop, "Workshop");

const Ogre::Real X_OFFSET_CREATURE = 0.7;
const Ogre::Real Y_OFFSET_CREATURE = 0.0;
const Ogre::Real X_OFFSET_SPOT = 0.0;
const Ogre::Real Y_OFFSET_SPOT = 0.2;

RoomWorkshop::RoomWorkshop(GameMap* gameMap) :
    Room(gameMap),
    mPoints(0),
    mTrapType(TrapType::nullTrapType)
{
    setMeshName("Workshop");
}

RenderedMovableEntity* RoomWorkshop::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    switch(place)
    {
        case ActiveSpotPlace::activeSpotCenter:
        {
            RoomWorkshopTileData* roomWorkshopTileData = static_cast<RoomWorkshopTileData*>(mTileData[tile]);
            roomWorkshopTileData->mCanHaveCraftedTrap = false;
            Ogre::Real x = static_cast<Ogre::Real>(tile->getX()) + X_OFFSET_SPOT;
            Ogre::Real y = static_cast<Ogre::Real>(tile->getY()) + Y_OFFSET_SPOT;
            mUnusedSpots.push_back(tile);
            int result = Random::Int(0, 3);
            if(result < 2)
                return loadBuildingObject(getGameMap(), "WorkshopMachine1", tile, x, y, 30.0, false);
            else
                return loadBuildingObject(getGameMap(), "WorkshopMachine2", tile, x, y, 30.0, false, 1.0, "Loop");
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
        default:
            break;
    }
    return nullptr;
}

void RoomWorkshop::absorbRoom(Room *r)
{
    if(r->getType() != getType())
    {
        OD_LOG_ERR("Trying to merge incompatible rooms: " + getName() + ", type=" + RoomManager::getRoomNameFromRoomType(getType()) + ", with " + r->getName() + ", type=" + RoomManager::getRoomNameFromRoomType(r->getType()));
        return;
    }
    RoomWorkshop* roomAbs = static_cast<RoomWorkshop*>(r);
    mUnusedSpots.insert(mUnusedSpots.end(), roomAbs->mUnusedSpots.begin(), roomAbs->mUnusedSpots.end());
    roomAbs->mUnusedSpots.clear();
    mCreaturesSpots.insert(roomAbs->mCreaturesSpots.begin(), roomAbs->mCreaturesSpots.end());
    roomAbs->mCreaturesSpots.clear();

    mPoints += roomAbs->mPoints;
    roomAbs->mPoints = 0;

    Room::absorbRoom(r);
}

void RoomWorkshop::notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
{
    Room::notifyActiveSpotRemoved(place, tile);

    if(place != ActiveSpotPlace::activeSpotCenter)
        return;

    RoomWorkshopTileData* roomWorkshopTileData = static_cast<RoomWorkshopTileData*>(mTileData[tile]);
    roomWorkshopTileData->mCanHaveCraftedTrap = true;

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
    if(itEr == mUnusedSpots.end())
    {
        OD_LOG_ERR("name=" + getName() + ", tile=" + Tile::displayAsString(tile));
        return;
    }
    mUnusedSpots.erase(itEr);
}

bool RoomWorkshop::hasOpenCreatureSpot(Creature* c)
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

bool RoomWorkshop::addCreatureUsingRoom(Creature* creature)
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
        if(pathToSpot.empty())
        {
            OD_LOG_ERR("unexpected empty pathToSpot");
            return true;
        }

        std::vector<Ogre::Vector3> path;
        Creature::tileToVector3(pathToSpot, path, true, 0.0);
        // We add the last step to take account of the offset
        Ogre::Vector3 dest(wantedX, wantedY, 0.0);
        path.push_back(dest);
        creature->setWalkPath(EntityAnimation::walk_anim, EntityAnimation::idle_anim, true, path);
    }

    return true;
}

void RoomWorkshop::removeCreatureUsingRoom(Creature* c)
{
    Room::removeCreatureUsingRoom(c);
    if(mCreaturesSpots.count(c) > 0)
    {
        Tile* tileSpot = mCreaturesSpots[c];
        if(tileSpot == nullptr)
        {
            OD_LOG_ERR("unexpected null tileSpot");
            return;
        }
        mUnusedSpots.push_back(tileSpot);
        mCreaturesSpots.erase(c);
    }
}

void RoomWorkshop::doUpkeep()
{
    Room::doUpkeep();

    if (mCoveredTiles.empty())
        return;

    // If we are not already working on something, we check if a trap have a need. If so,
    // we check that no reachable Workshop can supply the trap before starting crafting
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
        {
            OD_LOG_ERR("unexpected null tileCreature");
            continue;
        }

        Ogre::Real wantedX = -1;
        Ogre::Real wantedY = -1;
        getCreatureWantedPos(creature, tileSpot, wantedX, wantedY);

        RenderedMovableEntity* ro = getBuildingObjectFromTile(tileSpot);
        if(ro == nullptr)
        {
            OD_LOG_ERR("unexpected null building object");
            continue;
        }
        // We consider that the creature is in the good place if it is in the expected tile and not moving
        Tile* expectedDest = getGameMap()->getTile(Helper::round(wantedX), Helper::round(wantedY));
        if(expectedDest == nullptr)
        {
            OD_LOG_ERR("room=" + getName() + ", creature=" + creature->getName());
            continue;
        }
        if((tileCreature == expectedDest) &&
           !creature->isMoving())
        {
            if (creature->getJobCooldown() > 0)
            {
                creature->setAnimationState(EntityAnimation::idle_anim);
                creature->setJobCooldown(creature->getJobCooldown() - 1);
            }
            else
            {
                Ogre::Vector3 walkDirection(ro->getPosition().x - creature->getPosition().x - 1.0, ro->getPosition().y - creature->getPosition().y + 1.0, 0);
                walkDirection.normalise();
                creature->setAnimationState(EntityAnimation::attack_anim, false, walkDirection);

                ro->setAnimationState("Triggered", false);

                const CreatureRoomAffinity& creatureRoomAffinity = creature->getDefinition()->getRoomAffinity(getType());
                OD_ASSERT_TRUE_MSG(creatureRoomAffinity.getRoomType() == getType(), "name=" + getName() + ", creature=" + creature->getName()
                    + ", creatureRoomAffinityType=" + Helper::toString(static_cast<int>(creatureRoomAffinity.getRoomType())));

                mPoints += static_cast<int32_t>(creatureRoomAffinity.getEfficiency() * ConfigManager::getSingleton().getRoomConfigDouble("WorkshopPointsPerWork"));
                creature->jobDone(ConfigManager::getSingleton().getRoomConfigDouble("WorkshopAwaknessPerWork"));
                creature->setJobCooldown(Random::Uint(ConfigManager::getSingleton().getRoomConfigUInt32("WorkshopCooldownWorkMin"),
                    ConfigManager::getSingleton().getRoomConfigUInt32("WorkshopCooldownWorkMax")));
            }
        }
    }

    uint32_t nbCraftedItems = countCraftedItemsOnRoom();
    if(nbCraftedItems > (getNumActiveSpots() - mCreaturesSpots.size()))
    {
        // There is no available space. We remove a creature working here if there is one.
        // If there is none, it means the Workshop is full
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

    // We check if we have enough Workshop points for the currently crafted trap
    int32_t pointsNeeded = TrapManager::getNeededWorkshopPointsPerTrap(mTrapType);
    if(mPoints < pointsNeeded)
        return;

    // We check if there is an empty tile to release the craftedTrap
    Tile* tileCraftedTrap = checkIfAvailableSpot();
    if(tileCraftedTrap == nullptr)
    {
        OD_LOG_ERR("room=" + getName());
        return;
    }

    CraftedTrap* craftedTrap = new CraftedTrap(getGameMap(), true, getName(), mTrapType);
    craftedTrap->setSeat(getSeat());
    craftedTrap->addToGameMap();
    Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(tileCraftedTrap->getX()), static_cast<Ogre::Real>(tileCraftedTrap->getY()), static_cast<Ogre::Real>(0.0));
    craftedTrap->createMesh();
    craftedTrap->setPosition(spawnPosition);
    mPoints -= pointsNeeded;
    mTrapType = TrapType::nullTrapType;
}

uint32_t RoomWorkshop::countCraftedItemsOnRoom()
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

Tile* RoomWorkshop::checkIfAvailableSpot()
{
    for(std::pair<Tile* const, TileData*>& p : mTileData)
    {
        // If the tile contains no crafted trap, we can add a new one
        RoomWorkshopTileData* roomWorkshopTileData = static_cast<RoomWorkshopTileData*>(p.second);
        if(!roomWorkshopTileData->mCanHaveCraftedTrap)
            continue;

        bool isFilled = false;
        std::vector<GameEntity*> entities;
        p.first->fillWithCarryableEntities(entities);
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

        return p.first;
    }

    return nullptr;
}

void RoomWorkshop::getCreatureWantedPos(Creature* creature, Tile* tileSpot,
    Ogre::Real& wantedX, Ogre::Real& wantedY)
{
    wantedX = static_cast<Ogre::Real>(tileSpot->getX());
    wantedX += X_OFFSET_CREATURE;
    wantedY = static_cast<Ogre::Real>(tileSpot->getY());
    wantedY += Y_OFFSET_CREATURE;
}

void RoomWorkshop::exportToStream(std::ostream& os) const
{
    Room::exportToStream(os);
    os << mPoints << "\t";
    os << mTrapType << "\n";
}

void RoomWorkshop::importFromStream(std::istream& is)
{
    Room::importFromStream(is);
    OD_ASSERT_TRUE(is >> mPoints);
    OD_ASSERT_TRUE(is >> mTrapType);
}

RoomWorkshopTileData* RoomWorkshop::createTileData(Tile* tile)
{
    return new RoomWorkshopTileData;
}

void RoomWorkshop::checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    checkBuildRoomDefault(gameMap, RoomType::workshop, inputManager, inputCommand);
}

bool RoomWorkshop::buildRoom(GameMap* gameMap, Player* player, ODPacket& packet)
{
    std::vector<Tile*> tiles;
    if(!getRoomTilesDefault(tiles, gameMap, player, packet))
        return false;

    int32_t pricePerTarget = RoomManager::costPerTile(RoomType::workshop);
    int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
    if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
        return false;

    RoomWorkshop* room = new RoomWorkshop(gameMap);
    return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
}

bool RoomWorkshop::buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles)
{
    int32_t pricePerTarget = RoomManager::costPerTile(RoomType::crypt);
    int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
    if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
        return false;

    RoomWorkshop* room = new RoomWorkshop(gameMap);
    return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
}

void RoomWorkshop::checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    checkBuildRoomDefaultEditor(gameMap, RoomType::workshop, inputManager, inputCommand);
}

bool RoomWorkshop::buildRoomEditor(GameMap* gameMap, ODPacket& packet)
{
    RoomWorkshop* room = new RoomWorkshop(gameMap);
    return buildRoomDefaultEditor(gameMap, room, packet);
}

Room* RoomWorkshop::getRoomFromStream(GameMap* gameMap, std::istream& is)
{
    return new RoomWorkshop(gameMap);
}
