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

#include "ODPacket.h"
#include "RenderRequest.h"
#include "GameMap.h"
#include "RenderManager.h"
#include "TreasuryObject.h"
#include "LogManager.h"

#include <iostream>

const std::string RoomObject::ROOMOBJECT_PREFIX = "Room_Object_";
const std::string RoomObject::ROOMOBJECT_OGRE_PREFIX = "RoomObject_";

RoomObject::RoomObject(GameMap* gameMap, const std::string& baseName, const std::string& nMeshName, Ogre::Real rotationAngle) :
    MovableGameEntity(gameMap),
    mRotationAngle(rotationAngle),
    mIsOnMap(true)
{
    setObjectType(GameEntity::roomobject);
    setMeshName(nMeshName);
    // Set a unique name for the object
    setName(gameMap->nextUniqueNameRoomObj(baseName));
}

RoomObject::RoomObject(GameMap* gameMap) :
    MovableGameEntity(gameMap),
    mIsOnMap(true)
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

void RoomObject::pickup()
{
    mIsOnMap = false;
}

void RoomObject::setPosition(const Ogre::Vector3& v)
{
    MovableGameEntity::setPosition(v);
    mIsOnMap = true;
}

const char* RoomObject::getFormat()
{
    return "name\tmeshName";
}

RoomObject* RoomObject::getRoomObjectFromLine(GameMap* gameMap, const std::string& line)
{
    RoomObject* obj = nullptr;
    std::stringstream ss(line);
    RoomObject::RoomObjectType rot;
    OD_ASSERT_TRUE(ss >> rot);
    switch(rot)
    {
        case RoomObjectType::roomObject:
        {
            // Default type. Should not be saved in level file
            break;
        }
        case RoomObjectType::treasuryObject:
        {
            obj = TreasuryObject::getTreasuryObjectFromStream(gameMap, ss);
            break;
        }
        default:
        {
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(rot)));
            break;
        }
    }
    return obj;
}

RoomObject* RoomObject::getRoomObjectFromPacket(GameMap* gameMap, ODPacket& is)
{
    RoomObject* obj = nullptr;
    RoomObjectType rot;
    OD_ASSERT_TRUE(is >> rot);
    switch(rot)
    {
        case RoomObjectType::roomObject:
        {
            obj = new RoomObject(gameMap);
            OD_ASSERT_TRUE(is >> obj);
            break;
        }
        case RoomObjectType::treasuryObject:
        {
             obj = TreasuryObject::getTreasuryObjectFromPacket(gameMap, is);
            break;
        }
        default:
        {
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(rot)));
            break;
        }
    }
    return obj;
}

void RoomObject::exportToPacket(ODPacket& packet)
{
    packet << this;
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
    Ogre::Vector3 position;
    is >> name;
    ro->setName(name);
    is >> name;
    ro->setMeshName(name);
    is >> position;
    ro->setPosition(position);
    is >> ro->mRotationAngle;
    return is;
}

ODPacket& operator<<(ODPacket& os, const RoomObject::RoomObjectType& rot)
{
    uint32_t intType = static_cast<RoomObject::RoomObjectType>(rot);
    os << intType;
    return os;
}

ODPacket& operator>>(ODPacket& is, RoomObject::RoomObjectType& rot)
{
    uint32_t intType;
    is >> intType;
    rot = static_cast<RoomObject::RoomObjectType>(intType);
    return is;
}

std::ostream& operator<<(std::ostream& os, const RoomObject::RoomObjectType& rot)
{
    uint32_t intType = static_cast<RoomObject::RoomObjectType>(rot);
    os << intType;
    return os;
}

std::istream& operator>>(std::istream& is, RoomObject::RoomObjectType& rot)
{
    uint32_t intType;
    is >> intType;
    rot = static_cast<RoomObject::RoomObjectType>(intType);
    return is;
}
