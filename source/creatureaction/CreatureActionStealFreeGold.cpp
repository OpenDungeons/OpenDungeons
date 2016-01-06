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

#include "creatureaction/CreatureActionStealFreeGold.h"

#include "entities/Creature.h"
#include "entities/Tile.h"
#include "entities/TreasuryObject.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

// ATM, we use an hardcoded value for creatures stealing gold. Later, we might
// want to add something in the creature parameters or at least increase value
// for high tier/level creatures
const int GOLD_STEAL = 500;

std::function<bool()> CreatureActionStealFreeGold::action()
{
    return std::bind(&CreatureActionStealFreeGold::handleStealFreeGold,
        std::ref(mCreature));
}

bool CreatureActionStealFreeGold::handleStealFreeGold(Creature& creature)
{
    // Current creature tile position
    Tile* myTile = creature.getPositionTile();
    if (myTile == nullptr)
    {
        OD_LOG_ERR("name=" + creature.getName() + ", position=" + Helper::toString(creature.getPosition()));
        creature.popAction();
        return false;
    }

    // If we have been slapped, we cannot still gold
    if(creature.hasSlapEffect())
    {
        creature.popAction();
        return true;
    }

    // We look for the closest TreasuryObject
    TreasuryObject* treasuryClosest = nullptr;
    for(Tile* tile : creature.getVisibleTiles())
    {
        if(!creature.getGameMap()->pathExists(&creature, myTile, tile))
            continue;

        std::vector<GameEntity*> treasuryObjects;
        tile->fillWithEntities(treasuryObjects, SelectionEntityWanted::treasuryObjects, creature.getSeat()->getPlayer());
        if(treasuryObjects.empty())
            continue;

        for(GameEntity* entity : treasuryObjects)
        {
            TreasuryObject* treasuryObject = static_cast<TreasuryObject*>(entity);
            // We do not consider treasuries locked by a kobolds
            if(treasuryObject->getCarryLock(creature))
                continue;

            treasuryClosest = treasuryObject;
            break;
        }
        // getVisibleTiles returns tiles sorted by distance. Thanks to that,
        // we know the first we find will be the closest
        if(treasuryClosest != nullptr)
            break;
    }

    // If we found a treasury, we go to it
    if(treasuryClosest == nullptr)
    {
        // We couldn't find any treasury to take
        creature.popAction();
        return true;
    }

    // We check if we are on the same tile. If yes, we steal it. If not, we go there
    Tile* treasuryTile = treasuryClosest->getPositionTile();
    if (treasuryTile == nullptr)
    {
        OD_LOG_ERR("name=" + creature.getName() + ", position=" + Helper::toString(creature.getPosition()));
        creature.popAction();
        return false;
    }

    if(myTile == treasuryTile)
    {
        int gold = treasuryClosest->stealGold(creature, GOLD_STEAL);
        creature.addGoldCarried(gold);
        creature.popAction();
        return false;
    }

    if(!creature.setDestination(treasuryTile))
    {
        OD_LOG_ERR("creature=" + creature.getName() + ", myTile=" + Tile::displayAsString(myTile) + ", treasuryTile=" + Tile::displayAsString(treasuryTile));
        creature.popAction();
    }
    return false;
}
