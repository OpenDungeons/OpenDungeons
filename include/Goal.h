#ifndef GOAL_H
#define GOAL_H

#include <string>
using namespace std;

class Seat;
#include "Player.h"

class Goal
{
	public:
		// Constructors
		Goal(string nName, string nArguments, Player *nPlayer);

		// Functions which must be overridden by child classes
		virtual bool isMet(Seat *s) = 0;
		virtual bool isUnmet(Seat *s) = 0;
		virtual string getSuccessMessage() = 0;
		virtual string getDescription() = 0;

		// Functions which can be overridden (but do not have to be) by child classes
		virtual void doSuccessAction();
		virtual bool isVisible();

		// Functions which cannot be overridden by child classes
		string getName();

		friend ostream& operator<<(ostream& os, Goal *g);
		static Goal* instantiateFromStream(istream& is);

	protected:
		Player *player;

	private:
		string arguments;
		string name;
};

#include "Seat.h"

#include "GoalKillAllEnemies.h"
#include "GoalProtectCreature.h"

#endif

