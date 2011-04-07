#include "Globals.h"
#include "GoalProtectCreature.h"
#include "Player.h"
#include "GameMap.h"
#include "Creature.h"

GoalProtectCreature::GoalProtectCreature(const std::string& nName,
        const std::string& nArguments) :
    Goal(nName, nArguments)
{
    creatureName = nArguments;

    std::cout << "\nAdding goal " << getName() << "\tCreature name: "
            << creatureName;
}

bool GoalProtectCreature::isMet(Seat *s)
{
    Player *tempPlayer;

    // Check to see if the creature exists on the game map.
    Creature *tempCreature = gameMap.getCreature(creatureName);
    if (tempCreature != NULL)
    {
        return (tempCreature->getHP(NULL) > 0.0) ? true : false;
    }
    else
    {
        // The creature is not on the gameMap but it could be in one of the players hands.
        for (unsigned int i = 0, numP = gameMap.numPlayers(); i < numP; ++i)
        {
            tempPlayer = gameMap.getPlayer(i);
            for (unsigned int j = 0, numC = tempPlayer->numCreaturesInHand();
                    j < numC; ++j)
            {
                if (tempPlayer->getCreatureInHand(j)->getName() == creatureName)
                    return true;
            }
        }

        // The creature could be in my hand.
        tempPlayer = gameMap.me;
        for (unsigned int j = 0, numC = tempPlayer->numCreaturesInHand(); j < numC; ++j)
        {
            if (tempPlayer->getCreatureInHand(j)->getName() == creatureName)
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
    return creatureName + " is still alive";
}

std::string GoalProtectCreature::getFailedMessage()
{
    return creatureName + " is not alive";
}

std::string GoalProtectCreature::getDescription()
{
    return "Protect the creature named " + creatureName + ".";
}

