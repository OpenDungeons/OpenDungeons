#include <iostream>

//#include "Functions.h"
#include "RoomObject.h"
#include "RenderRequest.h"
#include "Room.h"
#include "GameMap.h"
#include "RenderManager.h"

RoomObject::RoomObject(Room* nParentRoom, const std::string& nMeshName) :
        parentRoom(nParentRoom),
        meshExists(false)
{
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

void RoomObject::createMesh()
{
    if (meshExists)
        return;

    meshExists = true;

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::createRoomObject;
    request->p = this;
    request->p2 = parentRoom;
    request->str = getName();
    request->p3 = new std::string(getMeshName());

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);

    parentRoom->getGameMap()->addAnimatedObject(this);
}

void RoomObject::destroyMesh()
{
    if (!meshExists)
        return;

    meshExists = false;

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::destroyRoomObject;
    request->p = this;
    request->p2 = parentRoom;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);

    parentRoom->getGameMap()->removeAnimatedObject(this);
}

void RoomObject::deleteYourself()
{
    if (meshExists)
        destroyMesh();

    // Create a render request asking the render queue to actually do the deletion of this creature.
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::deleteRoomObject;
    request->p = this;

    // Add the requests to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
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

