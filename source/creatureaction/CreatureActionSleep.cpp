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

#include "creatureaction/CreatureActionSleep.h"

#include "creatureaction/CreatureActionFindHome.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/Tile.h"
#include "rooms/Room.h"
#include "rooms/RoomDormitory.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"

std::function<bool()> CreatureActionSleep::action()
{
    return std::bind(&CreatureActionSleep::handleSleep,
        std::ref(mCreature), getNbTurnsActive());
}

bool CreatureActionSleep::handleSleep(Creature& creature, int32_t nbTurnsActive)
{
    if (creature.getHomeTile() == nullptr)
    {
        if(!creature.hasActionBeenTried(CreatureActionType::findHome))
        {
            creature.pushAction(Utils::make_unique<CreatureActionFindHome>(creature, false));
            return true;
        }

        creature.popAction();
        return false;
    }

    Tile* myTile = creature.getPositionTile();
    if (myTile != creature.getHomeTile())
    {
        // Walk to the the home tile.
        if (creature.setDestination(creature.getHomeTile()))
            return false;
    }
    else
    {
        // We are at the home tile so sleep. If it is the first time we are sleeping,
        // we send the animation
        if(nbTurnsActive == 0)
        {
            Room* roomHomeTile = creature.getHomeTile()->getCoveringRoom();
            if((roomHomeTile == nullptr) || (roomHomeTile->getType() != RoomType::dormitory))
            {
                OD_LOG_ERR("creature=" + creature.getName() + ", wrong sleep tile on " + Tile::displayAsString(creature.getHomeTile()));
                creature.popAction();
                return false;
            }
            RoomDormitory* dormitory = static_cast<RoomDormitory*>(roomHomeTile);
            creature.setAnimationState(EntityAnimation::sleep_anim, false, dormitory->getSleepDirection(&creature), false);
        }

        // Improve wakefulness
        creature.increaseWakefulness(1.5);
        creature.setHP(creature.getHP() + creature.getDefinition()->getSleepHeal());

        creature.computeCreatureOverlayHealthValue();

        if (creature.getWakefulness() >= 100.0 && creature.getHP() >= creature.getMaxHp())
        {
            creature.popAction();
            return false;
        }
    }
    return false;
}
