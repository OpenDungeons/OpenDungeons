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

#include "creatureaction/CreatureActionClaimGroundTile.h"

#include "entities/Creature.h"
#include "entities/Tile.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

CreatureActionClaimGroundTile::CreatureActionClaimGroundTile(Creature& creature, Tile& tileClaim) :
    CreatureAction(creature),
    mTileClaim(tileClaim)
{
    mTileClaim.addWorkerClaiming(mCreature);
}

CreatureActionClaimGroundTile::~CreatureActionClaimGroundTile()
{
    mTileClaim.removeWorkerClaiming(mCreature);
}

std::function<bool()> CreatureActionClaimGroundTile::action()
{
    return std::bind(&CreatureActionClaimGroundTile::handleCreatureActionClaimGroundTile,
        std::ref(mCreature), std::ref(mTileClaim));
}

bool CreatureActionClaimGroundTile::handleCreatureActionClaimGroundTile(Creature& creature, Tile& tileClaim)
{
    Tile* myTile = creature.getPositionTile();
    if (myTile == nullptr)
    {
        OD_LOG_ERR("creature=" + creature.getName() + ", pos=" + Helper::toString(creature.getPosition()));
        creature.popAction();
        return false;
    }

    // We check if we are on the expected tile. If not, we go there
    if(myTile != &tileClaim)
    {
        if(!creature.setDestination(&tileClaim))
            creature.popAction();

        return true;
    }

    // We check if the tile is still claimable
    if(!myTile->isGroundClaimable(creature.getSeat()))
    {
        creature.popAction();
        return true;
    }

    creature.setAnimationState(EntityAnimation::claim_anim);
    myTile->claimForSeat(creature.getSeat(), creature.getClaimRate());
    creature.receiveExp(1.5 * (creature.getClaimRate() / (0.35 + 0.05 * creature.getLevel())));

    // Since we danced on a tile we are done for this turn
    return false;
}
