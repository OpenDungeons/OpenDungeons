#include <sstream>

#include "Globals.h"
#include "GoalClaimNTiles.h"

GoalClaimNTiles::GoalClaimNTiles(string nName, string nArguments) :
    Goal(nName, nArguments)
{
    cout << "\nAdding goal " << getName();
    numberOfTiles = atoi(nArguments.c_str());
}

bool GoalClaimNTiles::isMet(Seat *s)
{
    if (s->getNumClaimedTiles() >= numberOfTiles)
        return true;
    else
        return false;
}

string GoalClaimNTiles::getSuccessMessage()
{
    std::stringstream tempSS;
    tempSS << "You have claimed more than " << numberOfTiles << " tiles.";
    return tempSS.str();
}

string GoalClaimNTiles::getFailedMessage()
{
    std::stringstream tempSS;
    tempSS << "You have failed to claim more than " << numberOfTiles
            << " tiles.";
    return tempSS.str();
}

string GoalClaimNTiles::getDescription()
{
    std::stringstream tempSS;
    tempSS << "Claimed " << gameMap.me->seat->getNumClaimedTiles() << " of "
            << numberOfTiles << " tiles.";
    return tempSS.str();
}

