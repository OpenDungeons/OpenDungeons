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

#include "creatureaction/CreatureActionFlee.h"

#include "creatureaction/CreatureActionWalkToTile.h"
#include "entities/Creature.h"
#include "gamemap/GameMap.h"
#include "rooms/Room.h"
#include "rooms/RoomType.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"
#include "utils/Random.h"

static const int NB_TURN_FLEE_MAX = 5;

std::function<bool()> CreatureActionFlee::action()
{
    return std::bind(&CreatureActionFlee::handleFlee,
        std::ref(mCreature), getNbTurns());
}

bool CreatureActionFlee::handleFlee(Creature& creature, int32_t nbTurns)
{
    Tile* myTile = creature.getPositionTile();
    if(myTile == nullptr)
    {
        OD_LOG_ERR("creature=" + creature.getName() + ", position=" + Helper::toString(creature.getPosition()));
        creature.popAction();
        return false;
    }

    // We try to go as far as possible from the enemies within visible tiles. We will quit flee mode when there will be no more
    // enemy objects nearby or if we have already flee for too much time
    if ((creature.getVisibleEnemyObjects().empty()) || (nbTurns > NB_TURN_FLEE_MAX))
    {
        creature.popAction();
        return true;
    }

    // We try to go closer to the dungeon temple. If we are too near or if we cannot go there, we will flee randomly
    std::vector<Room*> tempRooms = creature.getGameMap()->getRoomsByTypeAndSeat(RoomType::dungeonTemple, creature.getSeat());
    tempRooms = creature.getGameMap()->getReachableRooms(tempRooms, myTile, &creature);
    if(!tempRooms.empty())
    {
        // We can go to one dungeon temple
        Room* room = tempRooms[Random::Int(0, tempRooms.size() - 1)];
        Tile* tile = room->getCoveredTile(0);
        std::list<Tile*> result = creature.getGameMap()->path(&creature, tile);
        // If we are not too near from the dungeon temple, we go there
        if(result.size() > 5)
        {
            result.resize(5);
            std::vector<Ogre::Vector3> path;
            creature.tileToVector3(result, path, true, 0.0);
            creature.setWalkPath(EntityAnimation::flee_anim, EntityAnimation::idle_anim, true, path);
            creature.pushAction(Utils::make_unique<CreatureActionWalkToTile>(creature));
            return false;
        }
    }

    // No dungeon temple is acessible or we are too near. We will wander randomly
    creature.wanderRandomly(EntityAnimation::flee_anim);
    return false;
}
