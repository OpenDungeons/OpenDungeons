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

#include "creatureaction/CreatureActionUseHatchery.h"

#include "creatureaction/CreatureActionEatChicken.h"
#include "creaturemood/CreatureMood.h"
#include "entities/ChickenEntity.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "game/Seat.h"
#include "rooms/Room.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"
#include "utils/Random.h"

CreatureActionUseHatchery::CreatureActionUseHatchery(Creature& creature, Room& room, bool forced) :
    CreatureAction(creature),
    mRoom(&room),
    mForced(forced)
{
    if(!mRoom->addCreatureUsingRoom(&mCreature))
    {
        OD_LOG_ERR("creature=" + mCreature.getName() + ", cannot work in hatchery=" + mRoom->getName());
    }
}

CreatureActionUseHatchery::~CreatureActionUseHatchery()
{
    if(mRoom != nullptr)
        mRoom->removeCreatureUsingRoom(&mCreature);
}

std::function<bool()> CreatureActionUseHatchery::action()
{
    return std::bind(&CreatureActionUseHatchery::handleUseHatchery,
        std::ref(mCreature), mRoom, mForced);
}

bool CreatureActionUseHatchery::handleUseHatchery(Creature& creature, Room* room, bool forced)
{
    // Current creature tile position
    Tile* myTile = creature.getPositionTile();
    if (myTile == nullptr)
    {
        creature.popAction();
        return false;
    }

    if(forced && creature.getHunger() < 5.0)
    {
        creature.popAction();
        return true;
    }

    if(!forced && (creature.getHunger() <= Random::Double(0.0, 15.0)))
    {
        creature.popAction();
        return true;
    }

    // If the hatchery is not available, we stop working
    if(room == nullptr)
    {
        creature.popAction();
        return true;
    }

    // We check if we are on one of the hatchery tiles. If not, we will go to one
    if(myTile->getCoveringRoom() != room)
    {
        Tile* dest = room->getCoveredTile(0);
        if(dest == nullptr)
        {
            // No tile to go to !
            OD_LOG_ERR("creature=" + creature.getName() + ", hatchery=" + room->getName());
            creature.popAction();
            return true;
        }

        if(!creature.setDestination(dest))
            creature.popAction();

        return true;
    }

    ChickenEntity* chickenClosest = nullptr;
    for(Tile* tile : creature.getTilesWithinSightRadius())
    {
        // If we are eating in a hatchery, we consider chickens on the hatchery only
        if(tile->getCoveringRoom() != room)
            continue;

        std::vector<GameEntity*> chickens;
        tile->fillWithChickenEntities(chickens);
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

    // If we cannot find any available chicken. We wait
    if(chickenClosest != nullptr)
    {
        creature.pushAction(Utils::make_unique<CreatureActionEatChicken>(creature, *chickenClosest));
        return true;
    }

    return false;
}

std::string CreatureActionUseHatchery::getListenerName() const
{
    return toString(getType()) + ", creature=" + mCreature.getName();
}

bool CreatureActionUseHatchery::notifyDead(GameEntity* entity)
{
    if(entity == mRoom)
    {
        mRoom = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionUseHatchery::notifyRemovedFromGameMap(GameEntity* entity)
{
    if(entity == mRoom)
    {
        mRoom = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionUseHatchery::notifyPickedUp(GameEntity* entity)
{
    // That should not happen since we are listening to a room
    OD_LOG_ERR(toString(getType()) + ", creature=" + mCreature.getName() + ", entity=" + entity->getName());
    return true;
}

bool CreatureActionUseHatchery::notifyDropped(GameEntity* entity)
{
    // That should not happen since we are listening to a room
    OD_LOG_ERR(toString(getType()) + ", creature=" + mCreature.getName() + ", entity=" + entity->getName());
    return true;
}
