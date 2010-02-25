#ifndef GOAKILLALLENEMIES_H
#define GOAKILLALLENEMIES_H

#include "Goal.h"

class GoalKillAllEnemies : public Goal
{
	public:
		GoalKillAllEnemies(string nName, string nArguments, Player *nPlayer);

		// Inherited functions
		bool isMet(Seat *s);
		string getSuccessMessage();
		string getDescription();
};

#endif

