#ifndef GOAL_H
#define GOAL_H

#include <string>

class Seat;
#include "Player.h"

class Goal
{
	public:
		// Constructors
		Goal(std::string nName, std::string nArguments);

		// Functions which must be overridden by child classes
		virtual bool isMet(Seat *s) = 0;
		virtual std::string getDescription() = 0;
		virtual std::string getSuccessMessage() = 0;
		virtual std::string getFailedMessage() = 0;

		// Functions which can be overridden (but do not have to be) by child classes
		virtual void doSuccessAction();
		virtual bool isVisible();
		virtual bool isUnmet(Seat *s);
		virtual bool isFailed(Seat *s);

		// Functions which cannot be overridden by child classes
		std::string getName();

		void addSuccessSubGoal(Goal *g);
		unsigned int numSuccessSubGoals();
		Goal* getSuccessSubGoal(int index);

		void addFailureSubGoal(Goal *g);
		unsigned int numFailureSubGoals();
		Goal* getFailureSubGoal(int index);

		static std::string getFormat();
		friend ostream& operator<<(ostream& os, Goal *g);
		static Goal* instantiateFromStream(istream& is);

	protected:
		std::string name;
		std::string arguments;
		std::vector<Goal*> successSubGoals;
		std::vector<Goal*> failureSubGoals;
};

#include "Seat.h"

#include "GoalKillAllEnemies.h"
#include "GoalProtectCreature.h"
#include "GoalClaimNTiles.h"
#include "GoalMineNGold.h"
#include "GoalProtectDungeonTemple.h"

#endif

