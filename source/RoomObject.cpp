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

RoomObject::RoomObject(Room* nParentRoom, const std::string& nMeshName) :
    mParentRoom(nParentRoom)
{
    setObjectType(GameEntity::roomobject);
    setMeshName(nMeshName);
    // Set a unique name for the room.
    static int uniqueNumber = 0;
    std::stringstream tempSS;
    tempSS << "Room_" << mParentRoom->getName() << "_Object_" << ++uniqueNumber;
    setName(tempSS.str());
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

std::ostream& operator<<(std::ostream& os, RoomObject *o)
{
    return os;
}

std::istream& operator>>(std::istream& is, RoomObject *o)
{
    return is;
}
