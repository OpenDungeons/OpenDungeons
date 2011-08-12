#include "Globals.h"
#include "GameMap.h"
#include "Creature.h"
#include "GoalKillAllEnemies.h"

#include <iostream>

GoalKillAllEnemies::GoalKillAllEnemies(const std::string& nName,
        const std::string& nArguments) :
    Goal(nName, nArguments)
{
    std::cout << "\nAdding goal " << getName();
}

bool GoalKillAllEnemies::isMet(Seat *s)
{
    bool enemiesFound = false;

    // Loop over all the creatures in the game map and check to see if any of them are of a different color than our seat.
    for (unsigned int i = 0, num = gameMap.numCreatures(); i < num; ++i)
    {
        if (gameMap.getCreature(i)->color != s->color)
        {
            enemiesFound = true;
            break;
        }
    }

    return !enemiesFound;
}

std::string GoalKillAllEnemies::getSuccessMessage()
{
    return "You have killed all the enemy creatures.";
}

std::string GoalKillAllEnemies::getFailedMessage()
{
    return "You have failed to kill all the enemy creatures.";
}

std::string GoalKillAllEnemies::getDescription()
{
    return "Kill all enemy creatures.";
}

