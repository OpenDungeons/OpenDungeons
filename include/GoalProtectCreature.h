#ifndef GOALPROTECTCREATURE_H
#define GOALPROTECTCREATURE_H

#include <string>

#include "Goal.h"

class GoalProtectCreature : public Goal
{
	public:
		GoalProtectCreature(string nName, string nArguments);

		// Inherited functions
		bool isMet(Seat *s);
		bool isUnmet(Seat *s);
		bool isFailed(Seat *s);
		string getDescription();
		string getSuccessMessage();
		string getFailedMessage();

	private:
		string creatureName;
};

#endif


