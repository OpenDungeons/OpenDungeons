#ifndef ROOMQUARTERS_H
#define ROOMQUARTERS_H

#include "Room.h"

class RoomQuarters : public Room
{
	public:
		RoomQuarters();

		// Functions overriding virtual functions in the Room base class.
		void doUpkeep();
};

#endif

