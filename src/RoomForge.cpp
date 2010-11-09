#include "Globals.h"
#include "RoomForge.h"

RoomForge::RoomForge()
	: Room()
{
}

void RoomForge::createMeshes()
{
	Room::createMeshes();

	Tile *centralTile = getCentralTile();
	Tile *topCenterTile = gameMap.getTile(centralTile->x, centralTile->y+1);
	Tile *bottomLeftTile = gameMap.getTile(centralTile->x-1, centralTile->y-1);
	Tile *bottomRightTile = gameMap.getTile(centralTile->x+1, centralTile->y-1);

	loadRoomObject("ForgeForgeObject", topCenterTile);
	loadRoomObject("ForgeAnvilObject", bottomLeftTile);
	loadRoomObject("ForgeTableObject", bottomRightTile);

	createRoomObjectMeshes();
}

