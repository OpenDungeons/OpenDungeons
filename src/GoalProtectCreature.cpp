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
        if (tempCreature->getHP(NULL) > 0.0)
            return true;
        else
            return false;
    }
    else
    {
        // The creature is not on the gameMap but it could be in one of the players hands.
        for (unsigned i = 0; i < gameMap.numPlayers(); ++i)
        {
            tempPlayer = gameMap.getPlayer(i);
            for (unsigned int j = 0; j < tempPlayer->numCreaturesInHand(); ++j)
            {
                Creature *tempCreature = tempPlayer->getCreatureInHand(j);
                if (tempCreature->name == creatureName)
                    return true;
            }
        }

        // The creature could be in my hand.
        tempPlayer = gameMap.me;
        for (unsigned int j = 0; j < tempPlayer->numCreaturesInHand(); ++j)
        {
            Creature *tempCreature = tempPlayer->getCreatureInHand(j);
            if (tempCreature->name == creatureName)
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
    return std::string("Protect the creature named ") + creatureName + ".";
}

