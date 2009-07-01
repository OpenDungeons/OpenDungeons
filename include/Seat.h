#ifndef SEAT_H
#define SEAT_H

#include <iostream>
using namespace std;

#include <Ogre.h>

class Seat
{
	public:
		int color;
		string faction;
		int startingX, startingY;
		Ogre::ColourValue colourValue;
		double mana;		/**< The amount of 'keeper mana' the player has. */
		double HP;		/**< The amount of 'keeper HP' the player has. */

		friend ostream& operator<<(ostream& os, Seat *s);
		friend istream& operator>>(istream& is, Seat *s);
};

#endif

