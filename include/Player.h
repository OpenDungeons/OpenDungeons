#ifndef PLAYER_H
#define PLAYER_H

#include <string>
using namespace std;

#include "Creature.h"

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
		int color;		/**< The color which identifies the players creatures and tiles. */
		int mana;		/**< The amount of 'keeper mana' the player has. */
		bool humanPlayer;	/**< True: player is human.    False: player is a computer. */
		//int goldInTreasury();
		//int oreInRefinery();
		//int ironInRefinery();

		unsigned int numCreaturesInHand();
		Creature *getCreatureInHand(int i);
		void addCreatureToHand(Creature *c);
		void removeCreatureFromHand(int i);

	private:
		vector<Creature*> creaturesInHand;
};

#endif

