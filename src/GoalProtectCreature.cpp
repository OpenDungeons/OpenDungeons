#include "Globals.h"
#include "GoalProtectCreature.h"

GoalProtectCreature::GoalProtectCreature(string nName, string nArguments, Player *nPlayer)
	: Goal(nName, nArguments, nPlayer)
{
	creatureName = nArguments;

	cout << "\nAdding goal " << getName() << "\tCreature name: " << creatureName;
}

bool GoalProtectCreature::isMet(Seat *s)
{
	bool creatureIsAlive;

	Creature *tempCreature = gameMap.getCreature(creatureName);
	if(tempCreature != NULL)
	{
		if(tempCreature->hp > 0.0)
			creatureIsAlive = true;
		else
			creatureIsAlive = false;
	}
	else
	{
		creatureIsAlive = false;
	}

	return creatureIsAlive;
}

bool GoalProtectCreature::isUnmet(Seat *s)
{
	return !isMet(s);
}

string GoalProtectCreature::getSuccessMessage()
{
	return creatureName + " is still alive";
}

string GoalProtectCreature::getDescription()
{
	return (string)"Protect the creature named " + creatureName + ".";
}


