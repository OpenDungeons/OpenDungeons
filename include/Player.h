#ifndef PLAYER_H
#define PLAYER_H

#include <string>
using namespace std;

#include "Creature.h"

class Player
{
	public:
		string nick;

		unsigned int numCreaturesInHand();
		Creature *getCreatureInHand(int i);
		void addCreatureToHand(Creature *c);
		void removeCreatureFromHand(int i);

	private:
		vector<Creature*> creaturesInHand;
};

#endif

