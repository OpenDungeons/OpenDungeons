#include "Globals.h"
#include "GoalKillAllEnemies.h"

GoalKillAllEnemies::GoalKillAllEnemies(string nName, string nArguments, Player *nPlayer)
	: Goal(nName, nArguments, nPlayer)
{
	cout << "\nAdding goal " << getName();
}

bool GoalKillAllEnemies::isMet(Seat *s)
{
	return false;
}

bool GoalKillAllEnemies::isVisible()
{
	return true;
}

string GoalKillAllEnemies::getSuccessMessage()
{
	return "Congratulations:  You have killed all enemy creatures.";
}

void GoalKillAllEnemies::doSuccessAction()
{
}

string GoalKillAllEnemies::getDescription()
{
	return "Kill all enemy creatures.";
}

