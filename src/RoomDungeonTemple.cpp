#include "RoomDungeonTemple.h"

RoomDungeonTemple::RoomDungeonTemple()
	: Room()
{
	type = dungeonTemple;
	waitTurns = 0;
}

void RoomDungeonTemple::createMeshes()
{
	Room::createMeshes();

	loadRoomObject("DungeonTempleObject");
	createRoomObjectMeshes();
}

void RoomDungeonTemple::destroyMeshes()
{
	Room::destroyMeshes();
}

/*! \brief Counts down a timer until it reaches 0, then it spawns a kobold of the color of this dungeon temple at the center of the dungeon temple, and resets the timer.
 *
 */
void RoomDungeonTemple::produceKobold()
{
	if(waitTurns <= 0)
	{
		waitTurns = 30;
		//TODO: Spawn a kobold here.
		std::cout << "\n\n\nSpawning a Kobold of color " << color << "\n\n\n";
	}
	else
	{
		waitTurns--;
	}
}

