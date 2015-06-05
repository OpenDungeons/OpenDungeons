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

#include "goals/GoalClaimNTiles.h"

#include "game/Player.h"
#include "game/Seat.h"

#include <sstream>
#include <iostream>

GoalClaimNTiles::GoalClaimNTiles(const std::string& nName,
        const std::string& nArguments) :
        Goal(nName, nArguments),
        mNumberOfTiles(atoi(nArguments.c_str()))
{
}

bool GoalClaimNTiles::isMet(const Seat &s, const GameMap&)
{
    return (s.getNumClaimedTiles() >= mNumberOfTiles);
}

std::string GoalClaimNTiles::getSuccessMessage(const Seat&)
{
    std::stringstream tempSS;
    tempSS << "You have claimed more than " << mNumberOfTiles << " tiles.";
    return tempSS.str();
}

std::string GoalClaimNTiles::getFailedMessage(const Seat&)
{
    std::stringstream tempSS;
    tempSS << "You have failed to claim more than " << mNumberOfTiles
            << " tiles.";
    return tempSS.str();
}

std::string GoalClaimNTiles::getDescription(const Seat& s)
{
    std::stringstream tempSS;
    tempSS << "Claimed " << s.getNumClaimedTiles() << " of "
            << mNumberOfTiles << " tiles.";
    return tempSS.str();
}
