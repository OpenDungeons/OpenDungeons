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

#include "creatureaction/CreatureAction.h"

#include "entities/Creature.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <istream>

std::string CreatureAction::toString(CreatureActionType actionType)
{
    switch (actionType)
    {
    case CreatureActionType::walkToTile:
        return "walkToTile";

    case CreatureActionType::fight:
        return "fight";

    case CreatureActionType::fightFriendly:
        return "fightFriendly";

    case CreatureActionType::searchTileToDig:
        return "searchTileToDig";

    case CreatureActionType::digTile:
        return "digTile";

    case CreatureActionType::searchWallTileToClaim:
        return "searchWallTileToClaim";

    case CreatureActionType::claimWallTile:
        return "claimWallTile";

    case CreatureActionType::searchGroundTileToClaim:
        return "searchGroundTileToClaim";

    case CreatureActionType::claimGroundTile:
        return "claimGroundTile";

    case CreatureActionType::findHome:
        return "findHome";

    case CreatureActionType::sleep:
        return "sleep";

    case CreatureActionType::searchJob:
        return "searchJob";

    case CreatureActionType::useRoom:
        return "useRoom";

    case CreatureActionType::searchFood:
        return "searchFood";

    case CreatureActionType::eatChicken:
        return "eatChicken";

    case CreatureActionType::flee:
        return "flee";

    case CreatureActionType::searchEntityToCarry:
        return "searchEntityToCarry";

    case CreatureActionType::grabEntity:
        return "grabEntity";

    case CreatureActionType::carryEntity:
        return "carryEntity";

    case CreatureActionType::getFee:
        return "getFee";

    case CreatureActionType::leaveDungeon:
        return "leaveDungeon";

    case CreatureActionType::stealFreeGold:
        return "stealFreeGold";

    default:
        assert(false);
        break;
    }

    return "unhandledAct=" + Helper::toString(static_cast<uint32_t>(actionType));
}
