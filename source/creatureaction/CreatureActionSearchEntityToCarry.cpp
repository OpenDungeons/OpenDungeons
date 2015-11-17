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

#include "creatureaction/CreatureActionSearchEntityToCarry.h"

#include "creatureaction/CreatureActionGrabEntity.h"
#include "entities/Building.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"
#include "utils/Random.h"

CreatureActionSearchEntityToCarry::CreatureActionSearchEntityToCarry(Creature& creature, bool forced) :
    CreatureAction(creature),
    mForced(forced)
{
    mCreature.getSeat()->getPlayer()->notifyWorkerAction(mCreature, getType());
}

CreatureActionSearchEntityToCarry::~CreatureActionSearchEntityToCarry()
{
    mCreature.getSeat()->getPlayer()->notifyWorkerStopsAction(mCreature, getType());
}

std::function<bool()> CreatureActionSearchEntityToCarry::action()
{
    return std::bind(&CreatureActionSearchEntityToCarry::handleSearchEntityToCarry,
        std::ref(mCreature), mForced);
}

bool CreatureActionSearchEntityToCarry::handleSearchEntityToCarry(Creature& creature, bool forced)
{
    Tile* myTile = creature.getPositionTile();
    if(myTile == nullptr)
    {
        OD_LOG_ERR("creature=" + creature.getName());
        creature.popAction();
        return true;
    }

    // We should not be carrying something here
    if(creature.getCarriedEntity() != nullptr)
    {
        OD_LOG_ERR("creature=" + creature.getName() + ", CarriedEntity=" + creature.getCarriedEntity()->getName() + ", myTile=" + Tile::displayAsString(myTile));
        creature.releaseCarriedEntity();
        creature.popAction();
        return true;
    }

    std::vector<Building*> buildings = creature.getGameMap()->getReachableBuildingsPerSeat(creature.getSeat(), myTile, &creature);
    std::vector<GameEntity*> carryableEntities = creature.getGameMap()->getCarryableEntities(&creature, creature.getTilesWithinSightRadius());
    std::vector<Tile*> carryableEntityInMyTileClients;
    std::vector<GameEntity*> availableEntities;
    EntityCarryType highestPriority = EntityCarryType::notCarryable;
    // If a carryable entity of highest priority is in my tile, I proceed it
    GameEntity* carryableEntityInMyTile = nullptr;
    for(GameEntity* entity : carryableEntities)
    {
        // We check that the entity is free to be carried
        if(entity->getCarryLock(creature))
            continue;
        // We check that the carryable entity is reachable
        Tile* carryableEntTile = entity->getPositionTile();
        if(!creature.getGameMap()->pathExists(&creature, myTile, carryableEntTile))
            continue;

        // If we are forced to carry something, we consider only entities on our tile
        if(forced && (myTile != carryableEntTile))
            continue;

        // We check if the current entity is highest or equal to the older one (if any)
        if(entity->getEntityCarryType(&creature) > highestPriority)
        {
            // We check if a buildings wants this entity
            std::vector<Tile*> tilesDest;
            for(Building* building : buildings)
            {
                if(building->hasCarryEntitySpot(entity))
                {
                    Tile* tile = building->getCoveredTile(0);
                    tilesDest.push_back(tile);
                }
            }

            if(!tilesDest.empty())
            {
                // We found a reachable building for a higher priority entity. We use this from now on
                carryableEntityInMyTile = nullptr;
                availableEntities.clear();
                availableEntities.push_back(entity);
                highestPriority = entity->getEntityCarryType(&creature);
                if(myTile == carryableEntTile)
                {
                    carryableEntityInMyTile = entity;
                    carryableEntityInMyTileClients = tilesDest;
                }
            }
        }
        else if(entity->getEntityCarryType(&creature) == highestPriority)
        {
            // We check if a buildings wants this entity
            std::vector<Tile*> tilesDest;
            for(Building* building : buildings)
            {
                if(building->hasCarryEntitySpot(entity))
                {
                    Tile* tile = building->getCoveredTile(0);
                    tilesDest.push_back(tile);
                }
            }

            if(!tilesDest.empty())
            {
                // We found a reachable building for a higher priority entity. We use this from now on
                availableEntities.push_back(entity);
                if((myTile == carryableEntTile) &&
                   (carryableEntityInMyTile == nullptr))
                {
                    carryableEntityInMyTile = entity;
                    carryableEntityInMyTileClients = tilesDest;
                }
            }
        }
        else
        {
            // Entity with lower priority. We don't proceed
        }
    }

    if(availableEntities.empty())
    {
        // No entity to carry. We can do something else
        creature.popAction();
        return true;
    }

    // If a carryable entity is in my tile, I take it
    if(carryableEntityInMyTile != nullptr)
    {
        creature.pushAction(Utils::make_unique<CreatureActionGrabEntity>(creature, *carryableEntityInMyTile));
        return true;
    }

    // We randomly choose one of the visible carryable entities
    uint32_t index = Random::Uint(0,availableEntities.size()-1);
    GameEntity* entity = availableEntities[index];
    creature.pushAction(Utils::make_unique<CreatureActionGrabEntity>(creature, *entity));
    return true;
}
