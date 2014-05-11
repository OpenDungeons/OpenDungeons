#include "RoomForge.h"
#include "Tile.h"
#include "GameMap.h"
#include "RoomObject.h"

RoomForge::RoomForge()
{
    mType = forge;
}

void RoomForge::createMesh()
{
    Room::createMesh();

    Tile *centralTile = getCentralTile();
    Tile *topCenterTile = getGameMap()->getTile(centralTile->x, centralTile->y + 1);
    Tile *bottomLeftTile = getGameMap()->getTile(centralTile->x - 1, centralTile->y
            - 1);
    Tile *bottomRightTile = getGameMap()->getTile(centralTile->x + 1, centralTile->y
            - 1);

    loadRoomObject("ForgeForgeObject", topCenterTile);
    loadRoomObject("ForgeAnvilObject", bottomLeftTile);
    loadRoomObject("ForgeTableObject", bottomRightTile);

    createRoomObjectMeshes();
}

