#include "Globals.h"
#include "GoalKillAllEnemies.h"

GoalKillAllEnemies::GoalKillAllEnemies(string nName, string nArguments)
	: Goal(nName, nArguments)
{
	cout << "\nAdding goal " << getName();
}

bool GoalKillAllEnemies::isMet(Seat *s)
{
	bool enemiesFound = false;

	// Loop over all the creatures in the game map and check to see if any of them are of a different color than our seat.
	for(unsigned int i = 0; i < gameMap.numCreatures(); i++)
	{
		if(gameMap.getCreature(i)->color != s->color)
		{
			enemiesFound = true;
			break;
		}
	}

	return !enemiesFound;
}

string GoalKillAllEnemies::getSuccessMessage()
{
	return "You have killed all the enemy creatures.";
}

string GoalKillAllEnemies::getFailedMessage()
{
	return "You have failed to kill all the enemy creatures.";
}

string GoalKillAllEnemies::getDescription()
{
	return "Kill all enemy creatures.";
}

