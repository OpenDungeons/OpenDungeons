#ifndef GOALPROTECTCREATURE_H
#define GOALPROTECTCREATURE_H

#include <string>

#include "Goal.h"

class GoalProtectCreature : public Goal
{
	public:
		GoalProtectCreature(string nName, string nArguments, Player *nPlayer);

		// Inherited functions
		bool isMet(Seat *s);
		bool isUnmet(Seat *s);
		string getSuccessMessage();
		string getDescription();

	private:
		string creatureName;
};

#endif


