#ifndef SEAT_H
#define SEAT_H

#include <iostream>
#include <vector>
using namespace std;

#include <Ogre.h>

#include "Goal.h"

class Seat
{
	public:
		int color;		/**< \brief The color index of the players sitting in this seat. */
		string faction;		/**< \brief The name of the faction that this seat is playing as. */
		int startingX;		/**< \brief The starting camera location (in tile coordinates) of this seat. */
		int startingY;		/**< \brief The starting camera location (in tile coordinates) of this seat. */
		Ogre::ColourValue colourValue;		/**< \brief The actual color that this color index translates into. */
		double mana;		/**< \brief The amount of 'keeper mana' the player has. */
		double HP;		/**< \brief The amount of 'keeper HP' the player has. */
		vector<Goal*> goals;	/**< \brief The currently unment goals for this seat, the first Seat to empty this wins. */

		friend ostream& operator<<(ostream& os, Seat *s);
		friend istream& operator>>(istream& is, Seat *s);
};

#endif

