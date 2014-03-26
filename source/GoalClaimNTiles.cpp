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

#include "GoalClaimNTiles.h"

#include "GameMap.h"
#include "Player.h"
#include "Seat.h"
#include "ODFrameListener.h"

#include <sstream>
#include <iostream>

GoalClaimNTiles::GoalClaimNTiles(const std::string& nName,
        const std::string& nArguments) :
        Goal(nName, nArguments),
        mNumberOfTiles(atoi(nArguments.c_str()))
{
    std::cout << "\nAdding goal " << getName();
}

bool GoalClaimNTiles::isMet(Seat *s)
{
    return (s->getNumClaimedTiles() >= mNumberOfTiles);
}

std::string GoalClaimNTiles::getSuccessMessage()
{
    std::stringstream tempSS;
    tempSS << "You have claimed more than " << mNumberOfTiles << " tiles.";
    return tempSS.str();
}

std::string GoalClaimNTiles::getFailedMessage()
{
    std::stringstream tempSS;
    tempSS << "You have failed to claim more than " << mNumberOfTiles
            << " tiles.";
    return tempSS.str();
}

std::string GoalClaimNTiles::getDescription()
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    if (!gameMap)
        return std::string();

    std::stringstream tempSS;
    tempSS << "Claimed " << gameMap->me->getSeat()->getNumClaimedTiles() << " of "
            << mNumberOfTiles << " tiles.";
    return tempSS.str();
}
