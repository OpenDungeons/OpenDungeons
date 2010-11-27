#include "RoomDojo.h"

RoomDojo::RoomDojo()
	: Room()
{
	type = dojo;
}

void RoomDojo::createMeshes()
{
	Room::createMeshes();

	loadRoomObject("DojoRockObject");
	createRoomObjectMeshes();
}

