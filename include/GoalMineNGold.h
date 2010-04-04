#ifndef GOALMINENGOLD_H
#define GOALMINENGOLD_H

#include "Goal.h"

class GoalMineNGold : public Goal
{
	public:
		GoalMineNGold(string nName, string nArguments);

		// Inherited functions
		bool isMet(Seat *s);
		string getDescription();
		string getSuccessMessage();
		string getFailedMessage();

	private:
		int goldToMine;
};

#endif

