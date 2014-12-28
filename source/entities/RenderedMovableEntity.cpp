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

#include "entities/RenderedMovableEntity.h"

#include "entities/BuildingObject.h"
#include "entities/TreasuryObject.h"
#include "entities/ChickenEntity.h"
#include "entities/MissileObject.h"
#include "entities/SmallSpiderEntity.h"
#include "entities/CraftedTrap.h"

#include "gamemap/GameMap.h"

#include "network/ODPacket.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"

#include "render/RenderManager.h"
#include "utils/LogManager.h"

#include <iostream>

const std::string RenderedMovableEntity::RENDEREDMOVABLEENTITY_PREFIX = "RenderedMovableEntity_";
const std::string RenderedMovableEntity::RENDEREDMOVABLEENTITY_OGRE_PREFIX = "OgreRenderedMovableEntity_";

const Ogre::Vector3 SCALE(0.7,0.7,0.7);

RenderedMovableEntity::RenderedMovableEntity(GameMap* gameMap, const std::string& baseName, const std::string& nMeshName,
        Ogre::Real rotationAngle, bool hideCoveredTile, float opacity) :
    MovableGameEntity(gameMap, opacity),
    mRotationAngle(rotationAngle),
    mHideCoveredTile(hideCoveredTile)
{
    setObjectType(GameEntity::renderedMovableEntity);
    setMeshName(nMeshName);
    // Set a unique name for the object
    setName(gameMap->nextUniqueNameRenderedMovableEntity(baseName));
}

RenderedMovableEntity::RenderedMovableEntity(GameMap* gameMap) :
    MovableGameEntity(gameMap, 1.0f),
    mRotationAngle(0.0),
    mHideCoveredTile(false)
{
    setObjectType(GameEntity::renderedMovableEntity);
}

const Ogre::Vector3& RenderedMovableEntity::getScale() const
{
    return SCALE;
}

void RenderedMovableEntity::createMeshLocal()
{
    MovableGameEntity::createMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    RenderManager::getSingleton().rrCreateRenderedMovableEntity(this);
}

void RenderedMovableEntity::destroyMeshLocal()
{
    MovableGameEntity::destroyMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    RenderManager::getSingleton().rrDestroyRenderedMovableEntity(this);
}

void RenderedMovableEntity::pickup()
{
    setIsOnMap(false);
    clearDestinations();
}

void RenderedMovableEntity::drop(const Ogre::Vector3& v)
{
    setIsOnMap(true);
    setPosition(v, false);
}

void RenderedMovableEntity::fireAddEntity(Seat* seat, bool async)
{
    if(async)
    {
        ServerNotification serverNotification(
            ServerNotification::addRenderedMovableEntity, seat->getPlayer());
        exportHeadersToPacket(serverNotification.mPacket);
        exportToPacket(serverNotification.mPacket);
        ODServer::getSingleton().sendAsyncMsg(serverNotification);
    }
    else
    {
        ServerNotification* serverNotification = new ServerNotification(
            ServerNotification::addRenderedMovableEntity, seat->getPlayer());
        exportHeadersToPacket(serverNotification->mPacket);
        exportToPacket(serverNotification->mPacket);
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

void RenderedMovableEntity::fireRemoveEntity(Seat* seat)
{
    ServerNotification *serverNotification = new ServerNotification(
        ServerNotification::removeRenderedMovableEntity, seat->getPlayer());
    const std::string& name = getName();
    serverNotification->mPacket << name;
    ODServer::getSingleton().queueServerNotification(serverNotification);
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
            OD_ASSERT_TRUE_MSG(false, "Building objects are not to be created from a line");
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
        case RenderedMovableEntityType::craftedTrap:
        {
            obj = CraftedTrap::getCraftedTrapFromStream(gameMap, is);
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
            obj = BuildingObject::getBuildingObjectFromPacket(gameMap, is);
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
        case RenderedMovableEntityType::smallSpiderEntity:
        {
            obj = SmallSpiderEntity::getSmallSpiderEntityFromPacket(gameMap, is);
            break;
        }
        case RenderedMovableEntityType::craftedTrap:
        {
            obj = CraftedTrap::getCraftedTrapFromPacket(gameMap, is);
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

void RenderedMovableEntity::exportToPacket(ODPacket& os) const
{
    std::string name = getName();
    std::string meshName = getMeshName();
    os << name << meshName;
    os << mPosition << mRotationAngle << mHideCoveredTile;

    MovableGameEntity::exportToPacket(os);
}

void RenderedMovableEntity::importFromPacket(ODPacket& is)
{
    std::string name;
    std::string meshName;
    OD_ASSERT_TRUE(is >> name);
    setName(name);
    OD_ASSERT_TRUE(is >> meshName);
    setMeshName(meshName);
    OD_ASSERT_TRUE(is >> mPosition);
    OD_ASSERT_TRUE(is >> mRotationAngle);

    OD_ASSERT_TRUE(is >> mHideCoveredTile);

    MovableGameEntity::importFromPacket(is);
}

void RenderedMovableEntity::exportToStream(std::ostream& os) const
{
    std::string name = getName();
    std::string meshName = getMeshName();
    os << name << "\t" << meshName << "\t";
    os << mPosition.x << "\t" << mPosition.y << "\t" << mPosition.z << "\t";
    os << mRotationAngle;

    MovableGameEntity::exportToStream(os);
}

void RenderedMovableEntity::importFromStream(std::istream& is)
{
    std::string name;
    std::string meshName;
    OD_ASSERT_TRUE(is >> name);
    setName(name);
    OD_ASSERT_TRUE(is >> meshName);
    setMeshName(meshName);
    OD_ASSERT_TRUE(is >> mPosition.x >> mPosition.y >> mPosition.z);
    OD_ASSERT_TRUE(is >> mRotationAngle);

    MovableGameEntity::importFromStream(is);
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
