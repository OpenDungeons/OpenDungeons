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

#include "GoalProtectCreature.h"

#include "Player.h"
#include "GameMap.h"
#include "Creature.h"
#include "ODFrameListener.h"

GoalProtectCreature::GoalProtectCreature(const std::string& nName, const std::string& nArguments) :
    Goal(nName, nArguments),
    mCreatureName(nArguments)
{
    std::cout << "\nAdding goal " << getName() << "\tCreature name: "
              << mCreatureName;
}

bool GoalProtectCreature::isMet(Seat *s)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    if (!gameMap)
        return false;

    // Check to see if the creature exists on the game map.
    const Creature *tempCreature = gameMap->getCreature(mCreatureName);
    if (tempCreature != NULL)
    {
        return (tempCreature->getHP() > 0.0);
    }
    else
    {
        // The creature is not on the gameMap but it could be in one of the players hands.
        for (unsigned int i = 0, numP = gameMap->numPlayers(); i < numP; ++i)
        {
            const Player *tempPlayer = gameMap->getPlayer(i);
            for (unsigned int j = 0, numC = tempPlayer->numCreaturesInHand();
                    j < numC; ++j)
            {
                if (tempPlayer->getCreatureInHand(j)->getName() == mCreatureName)
                    return true;
            }
        }

        // The creature could be in my hand.
        const Player *localPlayer = gameMap->getLocalPlayer();
        for (unsigned int j = 0, numC = localPlayer->numCreaturesInHand(); j < numC; ++j)
        {
            if (localPlayer->getCreatureInHand(j)->getName() == mCreatureName)
                return true;
        }

        return false;
    }
}

bool GoalProtectCreature::isUnmet(Seat *s)
{
    return false;
}

bool GoalProtectCreature::isFailed(Seat *s)
{
    return !isMet(s);
}

std::string GoalProtectCreature::getSuccessMessage()
{
    return mCreatureName + " is still alive";
}

std::string GoalProtectCreature::getFailedMessage()
{
    return mCreatureName + " is not alive";
}

std::string GoalProtectCreature::getDescription()
{
    return "Protect the creature named " + mCreatureName + ".";
}

