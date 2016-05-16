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

#include "creatureaction/CreatureActionGetFee.h"

#include "creatureaction/CreatureActionWalkToTile.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "rooms/Room.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"
#include "utils/Random.h"

std::function<bool()> CreatureActionGetFee::action()
{
    return std::bind(&CreatureActionGetFee::handleGetFee,
        std::ref(mCreature));
}

bool CreatureActionGetFee::handleGetFee(Creature& creature)
{
    Tile* myTile = creature.getPositionTile();
    if(myTile == nullptr)
    {
        OD_LOG_ERR("name=" + creature.getName() + ", position=" + Helper::toString(creature.getPosition()));
        creature.popAction();
        return false;
    }
    // We check if we are on a treasury. If yes, we try to take our fee
    if((myTile->getCoveringRoom() != nullptr) &&
       (creature.getSeat()->canOwnedCreatureUseRoomFrom(myTile->getCoveringRoom()->getSeat())))
    {
        int goldTaken = myTile->getCoveringRoom()->withdrawGold(creature.getGoldFee());
        if(goldTaken > 0)
        {
            creature.decreaseGoldFee(goldTaken);
            creature.addGoldCarried(goldTaken);
            creature.fireChatMsgTookFee(goldTaken);

            if(creature.getGoldFee() <= 0)
            {
                // We were able to take all the gold. We can do something else
                creature.popAction();
                return true;
            }
        }
    }

    // We try to go to some treasury were there is still some gold
    std::vector<Tile*> availableTreasuries;
    for(Room* room : creature.getGameMap()->getRooms())
    {
        if(room->getSeat() != creature.getSeat())
            continue;

        if(room->getTotalGoldStored() <= 0)
            continue;

        if(room->numCoveredTiles() <= 0)
            continue;

        Tile* tile = room->getCoveredTile(0);
        if(!creature.getGameMap()->pathExists(&creature, myTile, tile))
            continue;

        availableTreasuries.push_back(tile);
    }

    if(availableTreasuries.empty())
    {
        // No available treasury
        creature.popAction();
        return true;
    }

    Tile* chosenTile = nullptr;
    std::list<Tile*> tilePath = creature.getGameMap()->findBestPath(&creature, myTile,
        availableTreasuries, chosenTile);

    if(tilePath.empty() || (chosenTile == nullptr))
    {
        // No available treasury
        creature.popAction();
        return true;
    }

    std::vector<Ogre::Vector3> vectorPath;
    creature.tileToVector3(tilePath, vectorPath, true, 0.0);
    creature.setWalkPath(EntityAnimation::walk_anim, EntityAnimation::idle_anim, true, true, vectorPath);
    creature.pushAction(Utils::make_unique<CreatureActionWalkToTile>(creature));
    return false;
}
