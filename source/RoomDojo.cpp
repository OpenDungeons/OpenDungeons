#include "RoomDojo.h"

#include "Tile.h"
#include "GameMap.h"

RoomDojo::RoomDojo()
{
    type = dojo;
}

void RoomDojo::createMeshes()
{
    Room::createMeshes();

    loadRoomObject("TrainingDummy");

    Tile *centralTile = getCentralTile();
    //TODO - will this crash if it is outside the map?
    Tile *bottomLeftTile = gameMap->getTile(centralTile->x - 1, centralTile->y
            - 1);
    //Load another object if this tile is in the same room and available.
    if(bottomLeftTile->getCoveringRoom()->getType()
        && !bottomLeftTile->getCoveringTrap()
        && bottomLeftTile->getCoveringRoom()->getType() == getType())
    {
        loadRoomObject("TrainingPole", bottomLeftTile);
    }

    loadRoomObject("TrainingDummy", centralTile);
    
    
    createRoomObjectMeshes();
}

int RoomDojo::numOpenCreatureSlots()
{
    return 3 - numCreaturesUsingRoom();
}

