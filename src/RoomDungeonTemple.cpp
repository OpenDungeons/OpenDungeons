#include "RoomDungeonTemple.h"

RoomDungeonTemple::RoomDungeonTemple()
	: Room()
{
	type = dungeonTemple;
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

