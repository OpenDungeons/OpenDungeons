#ifndef PLAYER_H
#define PLAYER_H

#include <string>
using namespace std;

class Goal;
#include "Creature.h"
#include "Seat.h"

/*! \brief The player cleass contains information about a human, or computer, player in the game.
 *
 * When a new player joins a game being hosted on a server the server will
 * allocate a new Player structure and fill it in with the appropriate values.
 * Its relevant information will then be sent to the other players in the game
 * so they are aware of its presence.  In the future if we decide to do a
 * single player game, thiis is where the computer driven strategy AI
 * calculations will take place.
 */
class Player
{
	public:
		Player();
		string nick;		/**< The nickname used un chat, etc. */
		bool humanPlayer;	/**< True: player is human.    False: player is a computer. */
		//int goldInTreasury();
		//int oreInRefinery();
		//int ironInRefinery();

		// Public functions
		unsigned int numCreaturesInHand();
		Creature *getCreatureInHand(int i);
		void pickUpCreature(Creature *c);
		bool dropCreature(Tile *t);
		void rotateCreaturesInHand(int n);

		// Public data members
		Seat *seat;

	private:
		// Private functions
		void addCreatureToHand(Creature *c);	// Private, for other classes use pickUpCreature() instead.
		void removeCreatureFromHand(int i);	// Private, for other classes use dropCreature() instead.

		// Private datamembers
		vector<Creature*> creaturesInHand;
};

#endif

