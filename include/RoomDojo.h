#ifndef ROOMDOJO_H
#define ROOMDOJO_H

#include "Room.h"

class RoomDojo : public Room
{
	public:
		RoomDojo();
		void createMeshes();
		int numOpenCreatureSlots();
};

#endif

