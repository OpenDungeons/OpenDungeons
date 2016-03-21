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

#include "creatureaction/CreatureActionLeaveDungeon.h"

#include "entities/Creature.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "rooms/Room.h"
#include "rooms/RoomType.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

std::function<bool()> CreatureActionLeaveDungeon::action()
{
    return std::bind(&CreatureActionLeaveDungeon::handleLeaveDungeon,
        std::ref(mCreature));
}

bool CreatureActionLeaveDungeon::handleLeaveDungeon(Creature& creature)
{
    Tile* myTile = creature.getPositionTile();
    if(myTile == nullptr)
    {
        OD_LOG_ERR("creature=" + creature.getName() + ", pos=" + Helper::toString(creature.getPosition()));
        creature.popAction();
        return false;
    }

    // Check if we are on the central tile of a portal
    if((myTile->getCoveringRoom() != nullptr) &&
       (myTile->getCoveringRoom()->getType() == RoomType::portal) &&
       (creature.getSeat()->canOwnedCreatureUseRoomFrom(myTile->getCoveringRoom()->getSeat())))
    {
        if(myTile == myTile->getCoveringRoom()->getCentralTile())
        {
            creature.fireChatMsgLeftDungeon();
            OD_LOG_INF("creature=" + creature.getName() + " left its dungeon");

            // We are on the central tile. We can leave the dungeon
            // If the creature has a homeTile where it sleeps, its bed needs to be destroyed.
            creature.clearDestinations(EntityAnimation::idle_anim, true, true);

            // Remove the creature from the game map and into the deletion queue, it will be deleted
            // when it is safe, i.e. all other pointers to it have been wiped from the program.
            creature.removeFromGameMap();
            creature.deleteYourself();
            return false;
        }
    }

    // We try to go to the portal
    std::vector<Room*> tempRooms = creature.getGameMap()->getRoomsByTypeAndSeat(RoomType::portal, creature.getSeat());
    tempRooms = creature.getGameMap()->getReachableRooms(tempRooms, myTile, &creature);
    if(tempRooms.empty())
    {
        creature.popAction();
        return true;
    }

    creature.fireChatMsgLeavingDungeon();

    int index = Random::Int(0, tempRooms.size() - 1);
    Room* room = tempRooms[index];
    Tile* tile = room->getCentralTile();
    if(!creature.setDestination(tile))
    {
        OD_LOG_ERR("creature=" + creature.getName() + ", myTile=" + Tile::displayAsString(myTile) + " cannot reach " + Tile::displayAsString(tile));
        creature.popAction();
        return false;
    }
    return false;
}
