#include <sstream>
#include <iostream>

#include "Globals.h"
#include "GoalMineNGold.h"
#include "GameMap.h"
#include "Player.h"

GoalMineNGold::GoalMineNGold(const std::string& nName,
        const std::string& nArguments) :
    Goal(nName, nArguments)
{
    std::cout << "\nAdding goal " << getName();
    goldToMine = atoi(nArguments.c_str());
}

bool GoalMineNGold::isMet(Seat *s)
{
    return (s->goldMined >= goldToMine) ? true : false;
}

std::string GoalMineNGold::getDescription()
{
    std::stringstream tempSS;
    tempSS << "Mined " << gameMap.me->seat->goldMined << " of " << goldToMine
            << " gold coins.";
    return tempSS.str();
}

std::string GoalMineNGold::getSuccessMessage()
{
    std::stringstream tempSS;
    tempSS << "You have mined more than " << goldToMine << " gold coins.";
    return tempSS.str();
}

std::string GoalMineNGold::getFailedMessage()
{
    std::stringstream tempSS;
    tempSS << "You have failed to mine more than " << goldToMine
            << " gold coins.";
    return tempSS.str();
}

