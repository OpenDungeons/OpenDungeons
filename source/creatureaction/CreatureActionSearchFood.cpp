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

#include "creatureaction/CreatureActionSearchFood.h"

#include "creatureaction/CreatureActionEatChicken.h"
#include "creatureaction/CreatureActionUseRoom.h"
#include "entities/ChickenEntity.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "gamemap/Pathfinding.h"
#include "rooms/Room.h"
#include "rooms/RoomType.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"
#include "utils/Random.h"

std::function<bool()> CreatureActionSearchFood::action()
{
    return std::bind(&CreatureActionSearchFood::handleSearchFood,
        std::ref(mCreature), mForced);
}

bool CreatureActionSearchFood::handleSearchFood(Creature& creature, bool forced)
{
    // Current creature tile position
    Tile* myTile = creature.getPositionTile();
    if(myTile == nullptr)
    {
        OD_LOG_ERR("creature=" + creature.getName());
        creature.popAction();
        return true;
    }

    // Check if the creature needs to eat
    if(!creature.needsToEat(forced))
    {
        creature.popAction();
        return true;
    }

    // We start by checking if there is an available chicken nearby
    // Note that the tiles TilesWithinSightRadius are sorted by distance. That
    // means that the first chicken found will be the closest
    ChickenEntity* chickenClosest = nullptr;
    for(Tile* tile : creature.getTilesWithinSightRadius())
    {
        // We look for chickens not in a hatchery only
        if(tile->checkCoveringRoomType(RoomType::hatchery))
            continue;

        std::vector<GameEntity*> chickens;
        tile->fillWithEntities(chickens, SelectionEntityWanted::chicken, creature.getSeat()->getPlayer());
        if(chickens.empty())
            continue;

        for(GameEntity* chickenEnt : chickens)
        {
            ChickenEntity* chicken = static_cast<ChickenEntity*>(chickenEnt);
            if(chicken->getLockEat(creature))
                continue;

            chickenClosest = chicken;
            break;
        }
        if(chickenClosest != nullptr)
            break;
    }

    // If we found a chicken, we go for it
    if(chickenClosest != nullptr)
    {
        creature.pushAction(Utils::make_unique<CreatureActionEatChicken>(creature, *chickenClosest));
        return true;
    }

    // We couldn't find a wandering chicken. We look for a room where we can eat
    // Get the list of hatchery controlled by our seat and make sure there is at least one.
    std::vector<Room*> hatcheries = creature.getGameMap()->getRoomsByTypeAndSeat(RoomType::hatchery, creature.getSeat());
    if (hatcheries.empty())
    {
        creature.getSeat()->getPlayer()->notifyCreatureCannotFindFood(creature);
        creature.popAction();
        return true;
    }

    // Pick a hatchery where we can eat and try to walk to it.
    std::vector<Tile*> hatcheriesTiles;
    for(Room* hatcheryRoom : hatcheries)
    {
        if(hatcheryRoom->numCoveredTiles() <= 0)
            continue;

        if(!hatcheryRoom->hasOpenCreatureSpot(&creature))
            continue;

        Tile* tile = hatcheryRoom->getCoveredTile(0);
        if(tile == nullptr)
        {
            OD_LOG_ERR("creature=" + creature.getName() + ", hatchery=" + hatcheryRoom->getName());
            continue;
        }

        if(!creature.getGameMap()->pathExists(&creature, myTile, tile))
            continue;

        hatcheriesTiles.push_back(tile);
    }

    if(hatcheriesTiles.empty())
    {
        creature.getSeat()->getPlayer()->notifyCreatureCannotFindFood(creature);
        creature.popAction();
        return true;
    }

    Tile* chosenTile = nullptr;
    std::list<Tile*> pathToHatchery = creature.getGameMap()->findBestPath(&creature, myTile, hatcheriesTiles, chosenTile);
    if(chosenTile == nullptr)
    {
        // We couldn't find a path !
        OD_LOG_ERR("creature=" + creature.getName());
        creature.popAction();
        return true;
    }

    // Sanity check: check that the covering room is a hatchery as it should be
    if(!chosenTile->checkCoveringRoomType(RoomType::hatchery))
    {
        OD_LOG_ERR("creature=" + creature.getName() + ", tile=" + Tile::displayAsString(chosenTile));
        creature.popAction();
        return true;
    }

    creature.pushAction(Utils::make_unique<CreatureActionUseRoom>(creature, *chosenTile->getCoveringRoom(), forced));
    return true;
}
