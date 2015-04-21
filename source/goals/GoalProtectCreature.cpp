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

GoalProtectCreature::GoalProtectCreature(const std::string& nName, const std::string& nArguments) :
    Goal(nName, nArguments),
    mCreatureName(nArguments)
{
    std::cout << "\nAdding goal " << getName() << "\tCreature name: "
              << mCreatureName;
}

bool GoalProtectCreature::isMet(const Seat&, const GameMap& gameMap)
{
    // Check to see if the creature exists on the game map.
    const Creature *tempCreature = gameMap.getCreature(mCreatureName);
    if (tempCreature != nullptr)
        return (tempCreature->getHP() > 0.0);

    return false;
}

std::string GoalProtectCreature::getSuccessMessage(const Seat&)
{
    return mCreatureName + " is still alive";
}

std::string GoalProtectCreature::getFailedMessage(const Seat&)
{
    return mCreatureName + " is not alive";
}

std::string GoalProtectCreature::getDescription(const Seat&)
{
    return "Protect the creature named " + mCreatureName + ".";
}

