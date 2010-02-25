#ifndef GOAL_H
#define GOAL_H

#include <string>
using namespace std;

class Seat;
#include "Player.h"

class Goal
{
	public:
		Goal(string nName, string nArguments, Player *nPlayer);

		virtual bool isMet(Seat *s) = 0;
		virtual bool isVisible() = 0;
		virtual string getSuccessMessage() = 0;
		virtual void doSuccessAction() = 0;
		virtual string getDescription() = 0;
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

#endif

