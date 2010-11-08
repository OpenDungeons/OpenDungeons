#include "RoomDungeonTemple.h"

RoomDungeonTemple::RoomDungeonTemple()
	: Room()
{
}

void RoomDungeonTemple::createMeshes()
{
	Room::createMeshes();

	Tile *tempTile = getCentralTile();
	roomObjects[tempTile] = new RoomObject(this, "DungeonTempleObject");
	roomObjects[tempTile]->x = tempTile->x;
	roomObjects[tempTile]->y = tempTile->y;

	createRoomObjectMeshes();
}

void RoomDungeonTemple::destroyMeshes()
{
	Room::destroyMeshes();
}

