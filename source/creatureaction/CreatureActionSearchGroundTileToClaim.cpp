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

#include "creatureaction/CreatureActionSearchGroundTileToClaim.h"

#include "creatureaction/CreatureActionClaimGroundTile.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "gamemap/Pathfinding.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"

CreatureActionSearchGroundTileToClaim::CreatureActionSearchGroundTileToClaim(Creature& creature, bool forced) :
    CreatureAction(creature),
    mForced(forced)
{
    mCreature.getSeat()->getPlayer()->notifyWorkerAction(mCreature, getType());
}

CreatureActionSearchGroundTileToClaim::~CreatureActionSearchGroundTileToClaim()
{
    mCreature.getSeat()->getPlayer()->notifyWorkerStopsAction(mCreature, getType());
}

std::function<bool()> CreatureActionSearchGroundTileToClaim::action()
{
    return std::bind(&CreatureActionSearchGroundTileToClaim::handleSearchGroundTileToClaim,
        std::ref(mCreature), getNbTurns(), mForced);
}

bool CreatureActionSearchGroundTileToClaim::handleSearchGroundTileToClaim(Creature& creature, int32_t nbTurns, bool forced)
{
    if(creature.getClaimRate() <= 0.0)
        return false;

    Tile* myTile = creature.getPositionTile();
    if (myTile == nullptr)
    {
        OD_LOG_ERR("creature=" + creature.getName() + ", pos=" + Helper::toString(creature.getPosition()));
        creature.popAction();
        return false;
    }

    if(!forced)
    {
        // If we are claiming tiles for too long, we stop to check if there is
        // something else to do
        if(nbTurns >= Creature::NB_TURNS_BEFORE_CHECKING_TASK)
        {
            creature.popAction();
            return true;
        }
    }

    // See if the tile we are standing on can be claimed
    if ((myTile->isGroundClaimable(creature.getSeat())) &&
        (myTile->canWorkerClaim(creature)))
    {
        // Check to see if one of the tile's neighbors is claimed for our color
        for (Tile* tempTile : myTile->getAllNeighbors())
        {
            // Check to see if the current neighbor is a claimed ground tile
            if(tempTile->isFullTile())
                continue;
            if(!tempTile->isClaimedForSeat(creature.getSeat()))
                continue;
            if(tempTile->getClaimedPercentage() < 1.0)
                continue;

            // We found a neighbor that is claimed for our side than we can start
            // dancing on this tile.  If there is "left over" claiming that can be done
            // it will spill over into neighboring tiles until it is gone.
            creature.pushAction(Utils::make_unique<CreatureActionClaimGroundTile>(creature, *myTile));
            return true;
        }
    }

    // The tile we are standing on is already claimed or is not currently
    // claimable, find candidates for claiming.
    // Start by checking the neighbor tiles of the one we are already in
    std::vector<Tile*> neighbors = myTile->getAllNeighbors();
    std::random_shuffle(neighbors.begin(), neighbors.end());
    for(Tile* tile : neighbors)
    {
        // If the current neighbor is claimable, walk into it and skip to the end of this turn
        if(tile->isFullTile())
            continue;
        if(!tile->isGroundClaimable(creature.getSeat()))
            continue;
        if(!tile->canWorkerClaim(creature))
            continue;

        // The neighbor tile is a potential candidate for claiming, to be an actual candidate
        // though it must have a neighbor of its own that is already claimed for our side.
        for(Tile* neigh : tile->getAllNeighbors())
        {
            if(neigh->isFullTile())
                continue;
            if(!neigh->isClaimedForSeat(creature.getSeat()))
                continue;
            if(neigh->getClaimedPercentage() < 1.0)
                continue;

            // We lock the tile
            creature.pushAction(Utils::make_unique<CreatureActionClaimGroundTile>(creature, *tile));
            return true;
        }
    }

    // If we still haven't found a tile to claim, we try to take the closest one
    float distBest = -1;
    Tile* tileToClaim = nullptr;
    for (Tile* tile : creature.getTilesWithinSightRadius())
    {
        // if this tile is not fully claimed yet or the tile is of another player's color
        if(tile == nullptr)
            continue;
        if(tile->isFullTile())
            continue;
        if(!tile->isGroundClaimable(creature.getSeat()))
            continue;
        if(!creature.getGameMap()->pathExists(&creature, myTile, tile))
            continue;
        if(!tile->canWorkerClaim(creature))
            continue;

        float dist = Pathfinding::squaredDistanceTile(*myTile, *tile);
        if((distBest != -1) && (distBest <= dist))
            continue;

        // Check to see if one of the tile's neighbors is claimed for our color
        for (Tile* neigh : tile->getAllNeighbors())
        {
            if(neigh->isFullTile())
                continue;
            if(!neigh->isClaimedForSeat(creature.getSeat()))
                continue;
            if(neigh->getClaimedPercentage() < 1.0)
                continue;

            distBest = dist;
            tileToClaim = tile;
            break;
        }
    }

    // Check if we found a tile
    if(tileToClaim != nullptr)
    {
        // We lock the tile
        creature.pushAction(Utils::make_unique<CreatureActionClaimGroundTile>(creature, *tileToClaim));
        return true;
    }

    // We couldn't find a tile to claim so we do something else
    creature.popAction();
    return true;
}
