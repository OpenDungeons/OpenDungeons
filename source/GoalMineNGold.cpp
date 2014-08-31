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

#include "GoalMineNGold.h"

#include "GameMap.h"
#include "Player.h"
#include "Seat.h"

#include <sstream>
#include <iostream>

GoalMineNGold::GoalMineNGold(const std::string& nName, const std::string& nArguments, GameMap* gameMap) :
        Goal(nName, nArguments, gameMap),
        mGoldToMine(atoi(nArguments.c_str()))
{
    std::cout << "\nAdding goal " << getName();
}

bool GoalMineNGold::isMet(Seat *s)
{
    return (s->getGoldMined() >= mGoldToMine);
}

std::string GoalMineNGold::getDescription(Seat *s)
{
    std::stringstream tempSS;
    tempSS << "Mined " << s->mGoldMined << " of " << mGoldToMine
            << " gold coins.";
    return tempSS.str();
}

std::string GoalMineNGold::getSuccessMessage(Seat *s)
{
    std::stringstream tempSS;
    tempSS << "You have mined more than " << mGoldToMine << " gold coins.";
    return tempSS.str();
}

std::string GoalMineNGold::getFailedMessage(Seat *s)
{
    std::stringstream tempSS;
    tempSS << "You have failed to mine more than " << mGoldToMine
            << " gold coins.";
    return tempSS.str();
}

