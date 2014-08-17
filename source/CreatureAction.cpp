/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#include "CreatureAction.h"

#include "Creature.h"

CreatureAction::CreatureAction() :
    mType    (idle),
    mTile    (NULL),
    mCreature(NULL)
{
}

CreatureAction::CreatureAction(const ActionType nType, Tile* nTile, Creature* nCreature) :
    mType(nType),
    mTile(nTile),
    mCreature(nCreature)
{
}

std::string CreatureAction::toString() const
{
    switch (mType)
    {
    case walkToTile:
        return "walkToTile";

    case maneuver:
        return "maneuver";

    case digTile:
        return "digTile";

    case claimWallTile:
        return "claimWallTile";

    case claimTile:
        return "claimTile";

    case depositGold:
        return "depositGold";

    case attackObject:
        return "attackObject";

    case findHome:
        return "findHome";

    case sleep:
        return "sleep";

    case workdecided:
        return "workdecided";

    case workforced:
        return "workforced";

    case idle:
        return "idle";

    default:
        return "unhandledAct";
    }
}
