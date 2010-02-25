#include "Goal.h"

Goal::Goal(string nName, string nArguments, Player *nPlayer)
{
	name = nName;
	arguments = nArguments;
	player = nPlayer;
}

void Goal::doSuccessAction()
{
}

bool Goal::isVisible()
{
	return true;
}

string Goal::getName()
{
	return name;
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

	if(tempName.compare("KillAllEnemies") == 0)
	{
		tempGoal = new GoalKillAllEnemies(tempName, tempArguments, NULL);
	}

	return tempGoal;
}

