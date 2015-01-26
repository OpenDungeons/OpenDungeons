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

CreatureAction::CreatureAction(const ActionType actionType, GameEntity::ObjectType entityType, const std::string& entityName, Tile* tile) :
    mActionType(actionType),
    mEntityType(entityType),
    mEntityName(entityName),
    mTile(tile),
    mNbTurns(0)
{
    // We check mandatory items according to action type
    switch(mActionType)
    {
        case ActionType::attackObject:
            OD_ASSERT_TRUE(!mEntityName.empty());
            OD_ASSERT_TRUE(mEntityType != GameEntity::ObjectType::unknown);
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
    case walkToTile:
        return "walkToTile";

    case fight:
        return "fight";

    case digTile:
        return "digTile";

    case claimWallTile:
        return "claimWallTile";

    case claimTile:
        return "claimTile";

    case attackObject:
        return "attackObject";

    case findHome:
        return "findHome";

    case findHomeForced:
        return "findHomeForced";

    case sleep:
        return "sleep";

    case jobdecided:
        return "jobdecided";

    case jobforced:
        return "jobforced";

    case eatdecided:
        return "eatdecided";

    case eatforced:
        return "eatforced";

    case flee:
        return "flee";

    case carryEntity:
        return "carryEntity";

    case getFee:
        return "getFee";

    case idle:
        return "idle";

    case leaveDungeon:
        return "leaveDungeon";

    case fightNaturalEnemy:
        return "fightNaturalEnemy";
    }

    return "unhandledAct";
}
