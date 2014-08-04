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

ODPacket& operator<<(ODPacket& os, RoomObject* o)
{
    os << o->getName() << o->getMeshName();
    os << o->mX << o->mY << o->mRotationAngle;
    return os;
}

ODPacket& operator>>(ODPacket& is, RoomObject* o)
{
    std::string name;
    Ogre::Real tmpReal;
    is >> name;
    o->setName(name);
    is >> name;
    o->setMeshName(name);
    is >> tmpReal;
    o->mX = tmpReal;
    is >> tmpReal;
    o->mY = tmpReal;
    is >> tmpReal;
    o->mRotationAngle = tmpReal;
    return is;
}
