/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#include "creatureaction/CreatureActionGrabEntity.h"

#include "creatureaction/CreatureActionCarryEntity.h"
#include "entities/Building.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "gamemap/GameMap.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"

CreatureActionGrabEntity::CreatureActionGrabEntity(Creature& creature, GameEntity& entityToCarry) :
    CreatureAction(creature),
    mEntityToCarry(&entityToCarry)
{
    mEntityToCarry->addGameEntityListener(this);
    mEntityToCarry->setCarryLock(mCreature, true);
}

CreatureActionGrabEntity::~CreatureActionGrabEntity()
{
    if(mEntityToCarry != nullptr)
    {
        mEntityToCarry->removeGameEntityListener(this);
        mEntityToCarry->setCarryLock(mCreature, false);
    }
}

std::function<bool()> CreatureActionGrabEntity::action()
{
    return std::bind(&CreatureActionGrabEntity::handleGrabEntity,
        std::ref(mCreature), std::ref(mEntityToCarry));
}

bool CreatureActionGrabEntity::handleGrabEntity(Creature& creature, GameEntity* entityToCarry)
{
    Tile* myTile = creature.getPositionTile();
    if(myTile == nullptr)
    {
        OD_LOG_ERR("creature=" + creature.getName() + ", pos=" + Helper::toString(creature.getPosition()));
        creature.popAction();
        return false;
    }

    if(entityToCarry == nullptr)
    {
        // entity may be nullptr if it got rotten before we reach it
        creature.popAction();
        return false;
    }

    // We check if we are on the entity position tile. If yes, we carry it to a building
    // that wants it (if any)
    Tile* entityTile = entityToCarry->getPositionTile();
    if(entityTile == nullptr)
    {
        OD_LOG_ERR("creature=" + creature.getName() + ", entityToCarry=" + entityToCarry->getName() + ", entityToCarry pos=" + Helper::toString(entityToCarry->getPosition()));
        creature.popAction();
        return false;
    }

    if(entityTile != myTile)
    {
        // We try to go at the given entity
        creature.setDestination(entityTile);
        return false;
    }

    // We try to find a building that wants the entity
    std::vector<Building*> buildings = creature.getGameMap()->getReachableBuildingsPerSeat(creature.getSeat(), myTile, &creature);
    std::vector<Tile*> tilesDest;
    for(Building* building : buildings)
    {
        if(building->hasCarryEntitySpot(entityToCarry))
        {
            Tile* tile = building->getCoveredTile(0);
            tilesDest.push_back(tile);
        }
    }

    if(tilesDest.empty())
    {
        // No building wants this entity
        creature.popAction();
        return true;
    }

    Tile* tileBuilding = nullptr;
    creature.getGameMap()->findBestPath(&creature, myTile, tilesDest, tileBuilding);
    if(tileBuilding == nullptr)
    {
        // We couldn't find a way to the building
        creature.popAction();
        return true;
    }

    Building* buildingWants = tileBuilding->getCoveringBuilding();
    if(buildingWants == nullptr)
    {
        OD_LOG_ERR("creature=" + creature.getName() + ", entityToCarry=" + entityToCarry->getName() + ", tileBuilding=" + Tile::displayAsString(tileBuilding));
        creature.popAction();
        return false;
    }

    creature.popAction();
    creature.pushAction(Utils::make_unique<CreatureActionCarryEntity>(creature, *entityToCarry, *buildingWants));
    return true;
}

std::string CreatureActionGrabEntity::getListenerName() const
{
    return toString(getType()) + ", creature=" + mCreature.getName();
}

bool CreatureActionGrabEntity::notifyDead(GameEntity* entity)
{
    if(entity == mEntityToCarry)
    {
        mEntityToCarry->setCarryLock(mCreature, false);
        mEntityToCarry = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionGrabEntity::notifyRemovedFromGameMap(GameEntity* entity)
{
    if(entity == mEntityToCarry)
    {
        mEntityToCarry->setCarryLock(mCreature, false);
        mEntityToCarry = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionGrabEntity::notifyPickedUp(GameEntity* entity)
{
    if(entity == mEntityToCarry)
    {
        mEntityToCarry->setCarryLock(mCreature, false);
        mEntityToCarry = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionGrabEntity::notifyDropped(GameEntity* entity)
{
    // That should not happen. For now, we only require events for attacked creatures. And when they
    // are picked up, we should have cleared the action queue
    OD_LOG_ERR(toString(getType()) + ", creature=" + mCreature.getName() + ", entity=" + entity->getName());
    return true;
}
