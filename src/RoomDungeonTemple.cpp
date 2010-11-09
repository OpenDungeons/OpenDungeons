#include "RoomDungeonTemple.h"

RoomDungeonTemple::RoomDungeonTemple()
	: Room()
{
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

