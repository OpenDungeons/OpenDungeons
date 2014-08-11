/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "RoomObject.h"

#include "RenderRequest.h"
#include "Room.h"
#include "GameMap.h"
#include "RenderManager.h"

#include <iostream>

RoomObject::RoomObject(GameMap* gameMap, Room* nParentRoom, const std::string& nMeshName) :
    MovableGameEntity(gameMap),
    mParentRoom(nParentRoom)
{
    setObjectType(GameEntity::roomobject);
    setMeshName(nMeshName);
    // Set a unique name for the room.
    std::stringstream tempSS;
    tempSS << "Room_" << mParentRoom->getName() << "_Object_" << gameMap->nextUniqueNumberRoomObj();
    setName(tempSS.str());
}

RoomObject::RoomObject(GameMap* gameMap, Room* nParentRoom) :
    MovableGameEntity(gameMap),
    mParentRoom(nParentRoom)
{
    setObjectType(GameEntity::roomobject);
}

void RoomObject::createMeshLocal()
{
    MovableGameEntity::createMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequest;
    request->type   = RenderRequest::createRoomObject;
    request->p2     = getParentRoom();
    request->str    = getName();
    request->str2   = getMeshName();
    request->p      = static_cast<void*>(this);
    RenderManager::queueRenderRequest(request);
}

void RoomObject::destroyMeshLocal()
{
    MovableGameEntity::destroyMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequest;
    request->type   = RenderRequest::destroyRoomObject;
    request->p2     = getParentRoom();
    request->p      = static_cast<void*>(this);
    RenderManager::queueRenderRequest(request);
}

void RoomObject::deleteYourselfLocal()
{
    MovableGameEntity::deleteYourselfLocal();
    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequest;
    request->type   = RenderRequest::deleteRoomObject;
    request->p      = static_cast<void*>(this);
    RenderManager::queueRenderRequest(request);
}

Room* RoomObject::getParentRoom()
{
    return mParentRoom;
}

std::string RoomObject::getOgreNamePrefix()
{
    return "RoomObject_";
}

const char* RoomObject::getFormat()
{
    return "name\tmeshName";
}

ODPacket& operator<<(ODPacket& os, RoomObject* ro)
{
    std::string name = ro->getName();
    std::string meshName = ro->getMeshName();
    os << name << meshName;
    os << ro->mX << ro->mY << ro->mRotationAngle;
    return os;
}

ODPacket& operator>>(ODPacket& is, RoomObject* ro)
{
    std::string name;
    Ogre::Real tmpReal;
    is >> name;
    ro->setName(name);
    is >> name;
    ro->setMeshName(name);
    is >> tmpReal;
    ro->mX = tmpReal;
    is >> tmpReal;
    ro->mY = tmpReal;
    is >> tmpReal;
    ro->mRotationAngle = tmpReal;
    return is;
}
