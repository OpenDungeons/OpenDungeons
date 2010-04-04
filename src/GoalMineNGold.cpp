#include <sstream>
using namespace std;

#include "Globals.h"
#include "GoalMineNGold.h"

GoalMineNGold::GoalMineNGold(string nName, string nArguments)
	: Goal(nName, nArguments)
{
	cout << "\nAdding goal " << getName();
	goldToMine = atoi(nArguments.c_str());
}

bool GoalMineNGold::isMet(Seat *s)
{
	if(s->gold >= goldToMine)
		return true;
	else
		return false;
}

string GoalMineNGold::getDescription()
{
	stringstream tempSS;
	tempSS << "Mined " << gameMap.me->seat->gold << " of " << goldToMine << " gold coins.";
	return tempSS.str();
}

string GoalMineNGold::getSuccessMessage()
{
	stringstream tempSS;
	tempSS << "You have mined more than " << goldToMine << " gold coins.";
	return tempSS.str();
}

string GoalMineNGold::getFailedMessage()
{
	stringstream tempSS;
	tempSS << "You have failed to mine more than " << goldToMine << " gold coins.";
	return tempSS.str();
}

