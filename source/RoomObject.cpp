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
#include "GameMap.h"
#include "RenderManager.h"

#include <iostream>

const std::string RoomObject::ROOMOBJECT_PREFIX = "Room_Object_";

RoomObject::RoomObject(GameMap* gameMap, const std::string& baseName, const std::string& nMeshName) :
    MovableGameEntity(gameMap)
{
    setObjectType(GameEntity::roomobject);
    setMeshName(nMeshName);
    // Set a unique name for the object
    setName(gameMap->nextUniqueNameRoomObj(baseName));
}

RoomObject::RoomObject(GameMap* gameMap) :
    MovableGameEntity(gameMap)
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

const char* RoomObject::getFormat()
{
    return "name\tmeshName";
}

ODPacket& operator<<(ODPacket& os, RoomObject* ro)
{
    std::string name = ro->getName();
    std::string meshName = ro->getMeshName();
    Ogre::Vector3 position = ro->getPosition();
    os << name << meshName;
    os << position << ro->mRotationAngle;
    return os;
}

ODPacket& operator>>(ODPacket& is, RoomObject* ro)
{
    std::string name;
    Ogre::Real tmpReal;
    Ogre::Vector3 position;
    is >> name;
    ro->setName(name);
    is >> name;
    ro->setMeshName(name);
    is >> position;
    ro->setPosition(position);
    is >> tmpReal;
    ro->mRotationAngle = tmpReal;
    return is;
}
