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

#include "creatureaction/CreatureActionSearchJob.h"

#include "creatureaction/CreatureActionSearchFood.h"
#include "creatureaction/CreatureActionSleep.h"
#include "creatureaction/CreatureActionUseRoom.h"
#include "creaturemood/CreatureMood.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/Tile.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "rooms/Room.h"
#include "rooms/RoomType.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"
#include "utils/Random.h"

std::function<bool()> CreatureActionSearchJob::action()
{
    return std::bind(&CreatureActionSearchJob::handleSearchJob,
        std::ref(mCreature), mForced);
}

bool CreatureActionSearchJob::handleSearchJob(Creature& creature, bool forced)
{
    // Current creature tile position
    Tile* myTile = creature.getPositionTile();
    if (myTile == nullptr)
    {
        creature.popAction();
        return false;
    }

    // If we are unhappy, we do not want to work
    switch(creature.getMoodValue())
    {
        case CreatureMoodLevel::Upset:
        {
            // 20% chances of not working
            if(Random::Int(0, 100) < 20)
            {
                creature.popAction();
                return true;
            }
            break;
        }
        case CreatureMoodLevel::Angry:
        case CreatureMoodLevel::Furious:
        {
            // We don't work
            creature.popAction();
            return true;
        }
        default:
            // We can work
            break;
    }

    bool workForced = creature.isForcedToWork();
    // If we are sleepy, we go to bed unless we have been slapped
    if (!workForced && (Random::Double(20.0, 30.0) > creature.getWakefulness()))
    {
        creature.popAction();
        creature.pushAction(Utils::make_unique<CreatureActionSleep>(creature));
        return true;
    }

    // If we are hungry, we try to find food unless we have been slapped
    if (!workForced && (Random::Double(70.0, 80.0) < creature.getHunger()))
    {
        creature.popAction();
        creature.pushAction(Utils::make_unique<CreatureActionSearchFood>(creature, false));
        return true;
    }

    if(forced)
    {
        // We check if we can work in the given room
        Room* room = myTile->getCoveringRoom();
        for(const CreatureRoomAffinity& affinity : creature.getDefinition()->getRoomAffinity())
        {
            if(room == nullptr)
                continue;
            if(room->getType() != affinity.getRoomType())
                continue;
            if(affinity.getEfficiency() <= 0)
                continue;
            if(!creature.getSeat()->canOwnedCreatureUseRoomFrom(room->getSeat()))
                continue;

            // It is the room responsibility to test if the creature is suited for working in it
            if(room->hasOpenCreatureSpot(&creature))
            {
                creature.pushAction(Utils::make_unique<CreatureActionUseRoom>(creature, *room, forced));
                return false;
            }
            break;
        }

        // If we couldn't work on the room we were forced to, we stop trying
        creature.popAction();
        return true;
    }

    // We get the room we like the most. If we are on such a room, we start working if we can
    for(const CreatureRoomAffinity& affinity : creature.getDefinition()->getRoomAffinity())
    {
        // If likeness = 0, we don't consider working here
        if(affinity.getLikeness() <= 0)
            continue;

        // See if we are in the room we like the most. If yes and we can work, we stay. If no,
        // We check if there is such a room somewhere else where we can go
        if((myTile->getCoveringRoom() != nullptr) &&
           (myTile->getCoveringRoom()->getType() == affinity.getRoomType()) &&
           (creature.getSeat()->canOwnedCreatureUseRoomFrom(myTile->getCoveringRoom()->getSeat())))
        {
            Room* room = myTile->getCoveringRoom();
            // If the efficiency is 0 or the room is a hatchery, we only wander in the room
            if((affinity.getEfficiency() <= 0) ||
               (room->getType() == RoomType::hatchery))
            {
                int index = Random::Int(0, room->numCoveredTiles() - 1);
                Tile* tileDest = room->getCoveredTile(index);
                creature.setDestination(tileDest);
                return false;
            }

            // It is the room responsibility to test if the creature is suited for working in it
            if(room->hasOpenCreatureSpot(&creature))
            {
                creature.pushAction(Utils::make_unique<CreatureActionUseRoom>(creature, *room, forced));
                return true;
            }
        }

        // We are not in a room of the good type or we couldn't use it. We check if there is a reachable room
        // of the good type
        std::vector<Room*> rooms = creature.getGameMap()->getRoomsByTypeAndSeat(affinity.getRoomType(), creature.getSeat());
        rooms = creature.getGameMap()->getReachableRooms(rooms, myTile, &creature);
        std::random_shuffle(rooms.begin(), rooms.end());
        for(Room* room : rooms)
        {
            // If efficiency is 0, we just want to wander so no need to check if the room
            // is available
            if((affinity.getEfficiency() <= 0) ||
               room->hasOpenCreatureSpot(&creature))
            {
                int index = Random::Int(0, room->numCoveredTiles() - 1);
                Tile* tileDest = room->getCoveredTile(index);
                creature.setDestination(tileDest);
                return false;
            }
        }
    }

    // Default action
    creature.popAction();
    return true;
}
