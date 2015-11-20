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

#include "creatureaction/CreatureActionJob.h"

#include "creatureaction/CreatureActionSearchFood.h"
#include "creatureaction/CreatureActionSleep.h"
#include "creaturemood/CreatureMood.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "game/Seat.h"
#include "rooms/Room.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"
#include "utils/Random.h"

CreatureActionJob::CreatureActionJob(Creature& creature, Room& room) :
    CreatureAction(creature),
    mRoom(&room)
{
    if(!mRoom->addCreatureUsingRoom(&mCreature))
    {
        OD_LOG_ERR("creature=" + mCreature.getName() + ", cannot work in room=" + mRoom->getName());
    }
}

CreatureActionJob::~CreatureActionJob()
{
    if(mRoom != nullptr)
        mRoom->removeCreatureUsingRoom(&mCreature);
}

std::function<bool()> CreatureActionJob::action()
{
    return std::bind(&CreatureActionJob::handleJob,
        std::ref(mCreature), mRoom);
}

bool CreatureActionJob::handleJob(Creature& creature, Room* room)
{
    // Current creature tile position
    Tile* myTile = creature.getPositionTile();
    if (myTile == nullptr)
    {
        creature.popAction();
        return false;
    }

    // If the room is not available, we stop working
    if(room == nullptr)
    {
        creature.popAction();
        return true;
    }

    // If we are unhappy, we stop working
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

    // We check if we are on a room tile. If not, we will go to one
    if(myTile->getCoveringRoom() != room)
    {
        Tile* dest = room->getCoveredTile(0);
        if(dest == nullptr)
        {
            // No tile to go to !
            OD_LOG_ERR("creature=" + creature.getName() + ", room=" + room->getName());
            creature.popAction();
            return true;
        }

        if(!creature.setDestination(dest))
            creature.popAction();

        return true;
    }

    // If we are tired/hungry, we go to bed unless we are forced to work
    bool workForced = room->isForcedToWork(creature);
    if(!workForced)
        workForced = creature.isForcedToWork();

    if (!workForced && (Random::Double(20.0, 30.0) > creature.getWakefulness()))
    {
        creature.popAction();
        creature.pushAction(Utils::make_unique<CreatureActionSleep>(creature));
        return true;
    }

    // If we are hungry, we go to bed unless we have been slapped
    if (!workForced && (Random::Double(70.0, 80.0) < creature.getHunger()))
    {
        creature.popAction();
        creature.pushAction(Utils::make_unique<CreatureActionSearchFood>(creature, false));
        return true;
    }

    // TODO: call some room function to aware that we have nothing to do. That
    // will avoid the room to have to check for that
    return false;
}

std::string CreatureActionJob::getListenerName() const
{
    return toString(getType()) + ", creature=" + mCreature.getName();
}

bool CreatureActionJob::notifyDead(GameEntity* entity)
{
    if(entity == mRoom)
    {
        mRoom = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionJob::notifyRemovedFromGameMap(GameEntity* entity)
{
    if(entity == mRoom)
    {
        mRoom = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionJob::notifyPickedUp(GameEntity* entity)
{
    // That should not happen since we are listening to a room
    OD_LOG_ERR(toString(getType()) + ", creature=" + mCreature.getName() + ", entity=" + entity->getName());
    return true;
}

bool CreatureActionJob::notifyDropped(GameEntity* entity)
{
    // That should not happen since we are listening to a room
    OD_LOG_ERR(toString(getType()) + ", creature=" + mCreature.getName() + ", entity=" + entity->getName());
    return true;
}
