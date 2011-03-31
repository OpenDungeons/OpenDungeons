#include <sstream>
#include <iostream>

#include "Globals.h"
#include "GoalClaimNTiles.h"
#include "GameMap.h"
#include "Player.h"

GoalClaimNTiles::GoalClaimNTiles(const std::string& nName,
        const std::string& nArguments) :
    Goal(nName, nArguments)
{
    std::cout << "\nAdding goal " << getName();
    numberOfTiles = atoi(nArguments.c_str());
}

bool GoalClaimNTiles::isMet(Seat *s)
{
    if (s->getNumClaimedTiles() >= numberOfTiles)
        return true;
    else
        return false;
}

std::string GoalClaimNTiles::getSuccessMessage()
{
    std::stringstream tempSS;
    tempSS << "You have claimed more than " << numberOfTiles << " tiles.";
    return tempSS.str();
}

std::string GoalClaimNTiles::getFailedMessage()
{
    std::stringstream tempSS;
    tempSS << "You have failed to claim more than " << numberOfTiles
            << " tiles.";
    return tempSS.str();
}

std::string GoalClaimNTiles::getDescription()
{
    std::stringstream tempSS;
    tempSS << "Claimed " << gameMap.me->seat->getNumClaimedTiles() << " of "
            << numberOfTiles << " tiles.";
    return tempSS.str();
}

