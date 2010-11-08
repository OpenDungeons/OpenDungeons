#ifndef ROOMDUNGEONTEMPLE_H
#define ROOMDUNGEONTEMPLE_H

#include "Room.h"

class RoomDungeonTemple : public Room
{
	public:
		RoomDungeonTemple();

		void createMeshes();
		void destroyMeshes();
};

#endif

