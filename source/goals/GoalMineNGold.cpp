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

#include "goals/GoalMineNGold.h"

#include "game/Player.h"
#include "game/Seat.h"

#include <sstream>
#include <iostream>

GoalMineNGold::GoalMineNGold(const std::string& nName, const std::string& nArguments) :
        Goal(nName, nArguments),
        mGoldToMine(atoi(nArguments.c_str()))
{
}

bool GoalMineNGold::isMet(const Seat &s, const GameMap&)
{
    return (s.getGoldMined() >= mGoldToMine);
}

std::string GoalMineNGold::getDescription(const Seat &s)
{
    std::stringstream tempSS;
    tempSS << "Mined " << s.getGoldMined() << " of " << mGoldToMine
            << " gold coins.";
    return tempSS.str();
}

std::string GoalMineNGold::getSuccessMessage(const Seat &s)
{
    std::stringstream tempSS;
    tempSS << "You have mined more than " << mGoldToMine << " gold coins.";
    return tempSS.str();
}

std::string GoalMineNGold::getFailedMessage(const Seat &s)
{
    std::stringstream tempSS;
    tempSS << "You have failed to mine more than " << mGoldToMine
            << " gold coins.";
    return tempSS.str();
}

