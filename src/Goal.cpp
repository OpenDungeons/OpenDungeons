#include "Goal.h"

Goal::Goal(string nName, string nArguments)
{
	name = nName;
	arguments = nArguments;
}

void Goal::doSuccessAction()
{
}

bool Goal::isVisible()
{
	return true;
}

bool Goal::isUnmet(Seat *s)
{
	return !isMet(s);
}

bool Goal::isFailed(Seat *s)
{
	return false;
}

string Goal::getName()
{
	return name;
}

string Goal::getFormat()
{
        return "goalName\targuments";
}

ostream& operator<<(ostream& os, Goal *g)
{
	os << g->name << "\t";
	os << (g->arguments.size() > 0 ? g->arguments : "NULL") << "\n";
	return os;
}

Goal* Goal::instantiateFromStream(istream& is)
{
	string tempName, tempArguments;
	Goal *tempGoal = NULL;

	is >> tempName >> tempArguments;

	if(tempArguments.compare("NULL") == 0)
		tempArguments = "";

	// Parse the goal type name to find out what subclass of goal tempGoal should be instantiated as.
	if(tempName.compare("KillAllEnemies") == 0)
	{
		tempGoal = new GoalKillAllEnemies(tempName, tempArguments);
	}

	else if(tempName.compare("ProtectCreature") == 0)
	{
		tempGoal = new GoalProtectCreature(tempName, tempArguments);
	}

	else if(tempName.compare("ClaimNTiles") == 0)
	{
		tempGoal = new GoalClaimNTiles(tempName, tempArguments);
	}

	else if(tempName.compare("MineNGold") == 0)
	{
		tempGoal = new GoalMineNGold(tempName, tempArguments);
	}

	return tempGoal;
}

