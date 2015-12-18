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

#include "creatureaction/CreatureActionFindHome.h"

#include "creatureaction/CreatureActionWalkToTile.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "rooms/Room.h"
#include "rooms/RoomDormitory.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"

#include <functional>

std::function<bool()> CreatureActionFindHome::action()
{
    return std::bind(&CreatureActionFindHome::handleFindHome,
        std::ref(mCreature), mForced);
}

bool CreatureActionFindHome::handleFindHome(Creature& creature, bool forced)
{
    // Check to see if we are standing in an open dormitory tile that we can claim as our home.
    Tile* myTile = creature.getPositionTile();
    if (myTile == nullptr)
    {
        creature.popAction();
        return false;
    }

    if(!forced && (creature.getHomeTile() != nullptr))
    {
        creature.popAction();
        return false;
    }

    if((myTile->getCoveringRoom() != nullptr) &&
       (creature.getSeat()->canOwnedCreatureUseRoomFrom(myTile->getCoveringRoom()->getSeat())) &&
       (myTile->getCoveringRoom()->getType() == RoomType::dormitory))
    {
        Room* roomHomeTile = nullptr;
        if(creature.getHomeTile() != nullptr)
        {
            roomHomeTile = creature.getHomeTile()->getCoveringRoom();
            // Same dormitory nothing to do
            if(roomHomeTile == myTile->getCoveringRoom())
            {
                creature.popAction();
                return true;
            }
        }

        Tile* homeTile = static_cast<RoomDormitory*>(myTile->getCoveringBuilding())->claimTileForSleeping(myTile, &creature);
        if(homeTile != nullptr)
        {
            // We could install the bed in the dormitory. If we already had one, we remove it
            if(roomHomeTile != nullptr)
                static_cast<RoomDormitory*>(roomHomeTile)->releaseTileForSleeping(creature.getHomeTile(), &creature);

            creature.setHomeTile(homeTile);
            creature.popAction();
            return true;
        }

        // The tile where we are is not claimable. We search if there is another in this dormitory
        Tile* tempTile = static_cast<RoomDormitory*>(myTile->getCoveringBuilding())->getLocationForBed(&creature);
        if(tempTile != nullptr)
        {
            creature.setDestination(tempTile);
            return false;
        }
    }

    // If we found a tile to claim as our home in the above block
    // If we have been forced, we do not search in another dormitory
    if (forced || (creature.getHomeTile() != nullptr))
    {
        creature.popAction();
        return true;
    }

    // Check to see if we can walk to a dormitory that does have an open tile.
    std::vector<Room*> tempRooms = creature.getGameMap()->getRoomsByTypeAndSeat(RoomType::dormitory, creature.getSeat());
    std::vector<Tile*> availableDormitories;
    // TODO: use GameMap::findBestPath instead if this to avoid computing every path
    for (Room* room : tempRooms)
    {
        if(room->getType() != RoomType::dormitory)
        {
            OD_LOG_ERR("room=" + room->getName());
            continue;
        }

        RoomDormitory* dormitory = static_cast<RoomDormitory*>(room);
        Tile* tile = dormitory->getLocationForBed(&creature);
        if(tile == nullptr)
            continue;

        availableDormitories.push_back(tile);
    }

    // If we found a valid path to an open room in a dormitory, then start walking along it.
    if (availableDormitories.empty())
    {
        // If we got here there are no reachable dormitory that are unclaimed so we quit trying to find one.
        creature.getSeat()->getPlayer()->notifyCreatureCannotFindBed(creature);
        creature.popAction();
        return true;
    }

    Tile* choosenTile = nullptr;
    std::list<Tile*> tempPath = creature.getGameMap()->findBestPath(&creature, myTile, availableDormitories, choosenTile);
    std::vector<Ogre::Vector3> path;
    creature.tileToVector3(tempPath, path, true, 0.0);
    creature.setWalkPath(EntityAnimation::walk_anim, EntityAnimation::idle_anim, true, true, path);
    creature.pushAction(Utils::make_unique<CreatureActionWalkToTile>(creature));
    return false;
}
