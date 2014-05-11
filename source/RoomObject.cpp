#include <iostream>

//#include "ODServer.h"
#include "RoomObject.h"
#include "RenderRequest.h"
#include "Room.h"
#include "GameMap.h"
#include "RenderManager.h"

RoomObject::RoomObject(Room* nParentRoom, const std::string& nMeshName) :
        parentRoom(nParentRoom)
{
    setObjectType(GameEntity::roomobject);
    setMeshName(nMeshName);
    // Set a unique name for the room.
    static int uniqueNumber = 0;
    std::stringstream tempSS;
    tempSS << "Room_" << parentRoom->getName() << "_Object_" << ++uniqueNumber;
    setName(tempSS.str());
}

Room* RoomObject::getParentRoom()
{
    return parentRoom;
}

std::string RoomObject::getOgreNamePrefix()
{
    return "RoomObject_";
}

const char* RoomObject::getFormat()
{
    return "name\tmeshName";
}

std::ostream& operator<<(std::ostream& os, RoomObject *o)
{
    return os;
}

std::istream& operator>>(std::istream& is, RoomObject *o)
{
    return is;
}
