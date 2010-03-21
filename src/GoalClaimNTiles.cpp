#include <sstream>
using namespace std;

#include "Globals.h"

GoalClaimNTiles::GoalClaimNTiles(string nName, string nArguments)
	: Goal(nName, nArguments)
{
	cout << "\nAdding goal " << getName();
	numberOfTiles = atoi(nArguments.c_str());
}

bool GoalClaimNTiles::isMet(Seat *s)
{
	if(s->numClaimedTiles >= numberOfTiles)
		return true;
	else
		return false;
}

string GoalClaimNTiles::getSuccessMessage()
{
	stringstream tempSS;
	tempSS << "You have claimed more than " << numberOfTiles << " tiles.";
	return tempSS.str();
}

string GoalClaimNTiles::getDescription()
{
	stringstream tempSS;
	tempSS << "Claimed " << gameMap.me->seat->numClaimedTiles << " of " << numberOfTiles << " tiles.";
	return tempSS.str();
}

