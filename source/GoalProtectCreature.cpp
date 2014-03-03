#include "GoalProtectCreature.h"
#include "Player.h"
#include "GameMap.h"
#include "Creature.h"

GoalProtectCreature::GoalProtectCreature(const std::string& nName,
        const std::string& nArguments, const GameMap& gameMap) :
    Goal(nName, nArguments, gameMap),
    creatureName(nArguments)
{
    std::cout << "\nAdding goal " << getName() << "\tCreature name: "
            << creatureName;
}

bool GoalProtectCreature::isMet(Seat *s)
{
    // Check to see if the creature exists on the game map.
    const Creature *tempCreature = gameMap.getCreature(creatureName);
    if (tempCreature != NULL)
    {
        return (tempCreature->getHP() > 0.0);
    }
    else
    {
        // The creature is not on the gameMap but it could be in one of the players hands.
        for (unsigned int i = 0, numP = gameMap.numPlayers(); i < numP; ++i)
        {
            const Player *tempPlayer = gameMap.getPlayer(i);
            for (unsigned int j = 0, numC = tempPlayer->numCreaturesInHand();
                    j < numC; ++j)
            {
                if (tempPlayer->getCreatureInHand(j)->getName() == creatureName)
                    return true;
            }
        }

        // The creature could be in my hand.
        const Player *localPlayer = gameMap.getLocalPlayer();
        for (unsigned int j = 0, numC = localPlayer->numCreaturesInHand(); j < numC; ++j)
        {
            if (localPlayer->getCreatureInHand(j)->getName() == creatureName)
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

