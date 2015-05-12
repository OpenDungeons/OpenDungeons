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

#include "entities/CreatureAction.h"
#include "entities/Creature.h"

#include "utils/LogManager.h"

CreatureAction::CreatureAction(const CreatureActionType actionType, GameEntityType entityType, const std::string& entityName, Tile* tile) :
    mActionType(actionType),
    mEntityType(entityType),
    mEntityName(entityName),
    mTile(tile),
    mNbTurns(0),
    mNbTurnsActive(0)
{
    // We check mandatory items according to action type
    switch(mActionType)
    {
        case CreatureActionType::attackObject:
            OD_ASSERT_TRUE(!mEntityName.empty());
            OD_ASSERT_TRUE(mEntityType != GameEntityType::unknown);
            OD_ASSERT_TRUE(mTile != nullptr);
            break;

        default:
            break;
    }
}

std::string CreatureAction::toString() const
{
    switch (mActionType)
    {
    case CreatureActionType::walkToTile:
        return "walkToTile";

    case CreatureActionType::fight:
        return "fight";

    case CreatureActionType::digTile:
        return "digTile";

    case CreatureActionType::claimWallTile:
        return "claimWallTile";

    case CreatureActionType::claimTile:
        return "claimTile";

    case CreatureActionType::attackObject:
        return "attackObject";

    case CreatureActionType::findHome:
        return "findHome";

    case CreatureActionType::findHomeForced:
        return "findHomeForced";

    case CreatureActionType::sleep:
        return "sleep";

    case CreatureActionType::jobdecided:
        return "jobdecided";

    case CreatureActionType::jobforced:
        return "jobforced";

    case CreatureActionType::eatdecided:
        return "eatdecided";

    case CreatureActionType::eatforced:
        return "eatforced";

    case CreatureActionType::flee:
        return "flee";

    case CreatureActionType::carryEntity:
        return "carryEntity";

    case CreatureActionType::carryEntityForced:
        return "carryEntityForced";

    case CreatureActionType::getFee:
        return "getFee";

    case CreatureActionType::idle:
        return "idle";

    case CreatureActionType::leaveDungeon:
        return "leaveDungeon";

    case CreatureActionType::fightNaturalEnemy:
        return "fightNaturalEnemy";
    default:
        assert(false);
        break;
    }

    return "unhandledAct";
}
