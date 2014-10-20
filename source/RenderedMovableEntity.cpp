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

#include "RenderedMovableEntity.h"

#include "ODPacket.h"
#include "RenderRequest.h"
#include "GameMap.h"
#include "RenderManager.h"
#include "TreasuryObject.h"
#include "ChickenEntity.h"
#include "MissileObject.h"
#include "LogManager.h"

#include <iostream>

const std::string RenderedMovableEntity::RENDEREDMOVABLEENTITY_PREFIX = "RenderedMovableEntity_";
const std::string RenderedMovableEntity::RENDEREDMOVABLEENTITY_OGRE_PREFIX = "OgreRenderedMovableEntity_";

RenderedMovableEntity::RenderedMovableEntity(GameMap* gameMap, const std::string& baseName, const std::string& nMeshName, Ogre::Real rotationAngle) :
    MovableGameEntity(gameMap),
    mRotationAngle(rotationAngle),
    mIsOnMap(true)
{
    setObjectType(GameEntity::renderedMovableEntity);
    setMeshName(nMeshName);
    // Set a unique name for the object
    setName(gameMap->nextUniqueNameRenderedMovableEntity(baseName));
}

RenderedMovableEntity::RenderedMovableEntity(GameMap* gameMap) :
    MovableGameEntity(gameMap),
    mRotationAngle(0.0),
    mIsOnMap(true)
{
    setObjectType(GameEntity::renderedMovableEntity);
}

void RenderedMovableEntity::createMeshLocal()
{
    MovableGameEntity::createMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequest;
    request->type   = RenderRequest::createRenderedMovableEntity;
    request->str    = getName();
    request->str2   = getMeshName();
    request->p      = static_cast<void*>(this);
    RenderManager::queueRenderRequest(request);
}

void RenderedMovableEntity::destroyMeshLocal()
{
    MovableGameEntity::destroyMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequest;
    request->type   = RenderRequest::destroyRenderedMovableEntity;
    request->p      = static_cast<void*>(this);
    RenderManager::queueRenderRequest(request);
}

void RenderedMovableEntity::pickup()
{
    mIsOnMap = false;
    clearDestinations();
}

void RenderedMovableEntity::drop(const Ogre::Vector3& v)
{
    mIsOnMap = true;
    setPosition(v);
}

const char* RenderedMovableEntity::getFormat()
{
    return "name\tmeshName";
}

RenderedMovableEntity* RenderedMovableEntity::getRenderedMovableEntityFromLine(GameMap* gameMap, const std::string& line)
{
    RenderedMovableEntity* obj = nullptr;
    std::stringstream is(line);
    RenderedMovableEntity::RenderedMovableEntityType rot;
    OD_ASSERT_TRUE(is >> rot);
    switch(rot)
    {
        case RenderedMovableEntityType::buildingObject:
        {
            obj = new RenderedMovableEntity(gameMap);
            break;
        }
        case RenderedMovableEntityType::treasuryObject:
        {
            obj = TreasuryObject::getTreasuryObjectFromStream(gameMap, is);
            break;
        }
        case RenderedMovableEntityType::chickenEntity:
        {
            obj = ChickenEntity::getChickenEntityFromStream(gameMap, is);
            break;
        }
        case RenderedMovableEntityType::missileObject:
        {
            obj = MissileObject::getMissileObjectFromStream(gameMap, is);
            break;
        }
        default:
        {
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(rot)));
            break;
        }
    }

    if(obj == nullptr)
        return nullptr;

    obj->importFromStream(is);
    return obj;
}

RenderedMovableEntity* RenderedMovableEntity::getRenderedMovableEntityFromPacket(GameMap* gameMap, ODPacket& is)
{
    RenderedMovableEntity* obj = nullptr;
    RenderedMovableEntityType rot;
    OD_ASSERT_TRUE(is >> rot);
    switch(rot)
    {
        case RenderedMovableEntityType::buildingObject:
        {
            obj = new RenderedMovableEntity(gameMap);
            break;
        }
        case RenderedMovableEntityType::treasuryObject:
        {
            obj = TreasuryObject::getTreasuryObjectFromPacket(gameMap, is);
            break;
        }
        case RenderedMovableEntityType::chickenEntity:
        {
            obj = ChickenEntity::getChickenEntityFromPacket(gameMap, is);
            break;
        }
        case RenderedMovableEntityType::missileObject:
        {
            obj = MissileObject::getMissileObjectFromPacket(gameMap, is);
            break;
        }
        default:
        {
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(rot)));
            break;
        }
    }

    if(obj == nullptr)
        return nullptr;

    obj->importFromPacket(is);
    return obj;
}

void RenderedMovableEntity::exportHeadersToStream(std::ostream& os)
{
    os << getRenderedMovableEntityType() << "\t";
}

void RenderedMovableEntity::exportHeadersToPacket(ODPacket& os)
{
    os << getRenderedMovableEntityType();
}

void RenderedMovableEntity::exportToPacket(ODPacket& os)
{
    std::string name = getName();
    std::string meshName = getMeshName();
    Ogre::Vector3 position = getPosition();
    os << name << meshName;
    os << position << mRotationAngle;
}

void RenderedMovableEntity::importFromPacket(ODPacket& is)
{
    std::string name;
    std::string meshName;
    Ogre::Vector3 position;
    OD_ASSERT_TRUE(is >> name);
    setName(name);
    OD_ASSERT_TRUE(is >> meshName);
    setMeshName(meshName);
    OD_ASSERT_TRUE(is >> position);
    setPosition(position);
    OD_ASSERT_TRUE(is >> mRotationAngle);
}

void RenderedMovableEntity::exportToStream(std::ostream& os)
{
    std::string name = getName();
    std::string meshName = getMeshName();
    Ogre::Vector3 position = getPosition();
    os << name << "\t" << meshName << "\t";
    os << position.x << "\t" << position.y << "\t" << position.z << "\t";
    os << mRotationAngle << "\t";
}

void RenderedMovableEntity::importFromStream(std::istream& is)
{
    std::string name;
    std::string meshName;
    Ogre::Vector3 position;
    OD_ASSERT_TRUE(is >> name);
    setName(name);
    OD_ASSERT_TRUE(is >> meshName);
    setMeshName(meshName);
    OD_ASSERT_TRUE(is >> position.x >> position.y >> position.z);
    setPosition(position);
    OD_ASSERT_TRUE(is >> mRotationAngle);
}

ODPacket& operator<<(ODPacket& os, const RenderedMovableEntity::RenderedMovableEntityType& rot)
{
    uint32_t intType = static_cast<RenderedMovableEntity::RenderedMovableEntityType>(rot);
    os << intType;
    return os;
}

ODPacket& operator>>(ODPacket& is, RenderedMovableEntity::RenderedMovableEntityType& rot)
{
    uint32_t intType;
    is >> intType;
    rot = static_cast<RenderedMovableEntity::RenderedMovableEntityType>(intType);
    return is;
}

std::ostream& operator<<(std::ostream& os, const RenderedMovableEntity::RenderedMovableEntityType& rot)
{
    uint32_t intType = static_cast<RenderedMovableEntity::RenderedMovableEntityType>(rot);
    os << intType;
    return os;
}

std::istream& operator>>(std::istream& is, RenderedMovableEntity::RenderedMovableEntityType& rot)
{
    uint32_t intType;
    is >> intType;
    rot = static_cast<RenderedMovableEntity::RenderedMovableEntityType>(intType);
    return is;
}
