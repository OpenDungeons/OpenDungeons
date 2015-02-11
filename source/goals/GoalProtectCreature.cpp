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

#include "goals/GoalProtectCreature.h"

#include "entities/Creature.h"

#include "game/Player.h"
#include "gamemap/GameMap.h"

GoalProtectCreature::GoalProtectCreature(const std::string& nName, const std::string& nArguments, GameMap* gameMap) :
    Goal(nName, nArguments, gameMap),
    mCreatureName(nArguments)
{
    std::cout << "\nAdding goal " << getName() << "\tCreature name: "
              << mCreatureName;
}

bool GoalProtectCreature::isMet(Seat *s)
{
    // Check to see if the creature exists on the game map.
    const Creature *tempCreature = mGameMap->getCreature(mCreatureName);
    if (tempCreature != nullptr)
        return (tempCreature->getHP() > 0.0);

    return false;
}

bool GoalProtectCreature::isUnmet(Seat *s)
{
    return false;
}

bool GoalProtectCreature::isFailed(Seat *s)
{
    return !isMet(s);
}

std::string GoalProtectCreature::getSuccessMessage(Seat *s)
{
    return mCreatureName + " is still alive";
}

std::string GoalProtectCreature::getFailedMessage(Seat *s)
{
    return mCreatureName + " is not alive";
}

std::string GoalProtectCreature::getDescription(Seat *s)
{
    return "Protect the creature named " + mCreatureName + ".";
}

