#include "RoomDojo.h"

RoomDojo::RoomDojo()
{
    type = dojo;
}

void RoomDojo::createMeshes()
{
    Room::createMeshes();

    loadRoomObject("DojoRockObject");
    createRoomObjectMeshes();
}

int RoomDojo::numOpenCreatureSlots()
{
    return 3 - numCreaturesUsingRoom();
}

