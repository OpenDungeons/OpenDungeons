#include <sstream>
#include <iostream>

#include "Globals.h"
#include "GoalMineNGold.h"
#include "GameMap.h"
#include "Player.h"

GoalMineNGold::GoalMineNGold(const std::string& nName,
        const std::string& nArguments) :
        Goal(nName, nArguments),
        goldToMine(atoi(nArguments.c_str()))
{
    std::cout << "\nAdding goal " << getName();
}

bool GoalMineNGold::isMet(Seat *s)
{
    return (s->goldMined >= goldToMine);
}

std::string GoalMineNGold::getDescription()
{
    std::stringstream tempSS;
    tempSS << "Mined " << gameMap.me->getSeat()->goldMined << " of " << goldToMine
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

