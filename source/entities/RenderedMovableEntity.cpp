/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
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
#include "entities/GameEntityType.h"
#include "entities/MissileObject.h"
#include "entities/SmallSpiderEntity.h"
#include "entities/CraftedTrap.h"
#include "entities/PersistentObject.h"
#include "entities/Tile.h"
#include "entities/TrapEntity.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "render/RenderManager.h"
#include "spells/Spell.h"
#include "utils/LogManager.h"
#include "utils/Helper.h"

#include <istream>
#include <ostream>

const std::string RenderedMovableEntity::RENDEREDMOVABLEENTITY_PREFIX = "RenderedMovableEntity_";

static const Ogre::Vector3 SCALE(0.7,0.7,0.7);

RenderedMovableEntity::RenderedMovableEntity(GameMap* gameMap, const std::string& baseName, const std::string& nMeshName,
        Ogre::Real rotationAngle, bool hideCoveredTile, float opacity) :
    MovableGameEntity(gameMap),
    mBaseName(baseName),
    mRotationAngle(rotationAngle),
    mHideCoveredTile(hideCoveredTile),
    mOpacity(opacity)
{
    setMeshName(nMeshName);
    // Set a unique name for the object
    setName(gameMap->nextUniqueNameRenderedMovableEntity(baseName));
}

RenderedMovableEntity::RenderedMovableEntity(GameMap* gameMap) :
    MovableGameEntity(gameMap),
    mRotationAngle(0.0),
    mHideCoveredTile(false),
    mOpacity(1.0f)
{
}

const Ogre::Vector3& RenderedMovableEntity::getScale() const
{
    return SCALE;
}

void RenderedMovableEntity::createMeshLocal()
{
    MovableGameEntity::createMeshLocal();

    if(getIsOnServerMap())
        return;

    RenderManager::getSingleton().rrCreateRenderedMovableEntity(this);
}

void RenderedMovableEntity::destroyMeshLocal()
{
    MovableGameEntity::destroyMeshLocal();

    if(getIsOnServerMap())
        return;

    RenderManager::getSingleton().rrDestroyRenderedMovableEntity(this);
}

void RenderedMovableEntity::addToGameMap()
{
    getGameMap()->addRenderedMovableEntity(this);
    getGameMap()->addAnimatedObject(this);
    getGameMap()->addClientUpkeepEntity(this);

    if(!getIsOnServerMap())
        return;

    getGameMap()->addActiveObject(this);
}

void RenderedMovableEntity::removeFromGameMap()
{
    fireEntityRemoveFromGameMap();
    removeEntityFromPositionTile();
    getGameMap()->removeRenderedMovableEntity(this);
    getGameMap()->removeAnimatedObject(this);
    getGameMap()->removeClientUpkeepEntity(this);

    if(!getIsOnServerMap())
        return;

    fireRemoveEntityToSeatsWithVision();

    getGameMap()->removeActiveObject(this);
}

void RenderedMovableEntity::setMeshOpacity(float opacity)
{
    if (opacity < 0.0f || opacity > 1.0f)
        return;

    mOpacity = opacity;

    if(getIsOnServerMap())
    {
        for(Seat* seat : mSeatsWithVisionNotified)
        {
            if(seat->getPlayer() == nullptr)
                continue;
            if(!seat->getPlayer()->getIsHuman())
                continue;

            ServerNotification* serverNotification = new ServerNotification(
                ServerNotificationType::setEntityOpacity, seat->getPlayer());
            const std::string& name = getName();
            serverNotification->mPacket << name << opacity;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
        return;
    }

    RenderManager::getSingleton().rrUpdateEntityOpacity(this);
}

void RenderedMovableEntity::pickup()
{
    removeEntityFromPositionTile();
    clearDestinations(EntityAnimation::idle_anim, true, true);
}

void RenderedMovableEntity::drop(const Ogre::Vector3& v)
{
    setPosition(v);
}

void RenderedMovableEntity::fireAddEntity(Seat* seat, bool async)
{
    if(async)
    {
        ServerNotification serverNotification(
            ServerNotificationType::addEntity, seat->getPlayer());
        exportHeadersToPacket(serverNotification.mPacket);
        exportToPacket(serverNotification.mPacket, seat);
        ODServer::getSingleton().sendAsyncMsg(serverNotification);
    }
    else
    {
        ServerNotification* serverNotification = new ServerNotification(
            ServerNotificationType::addEntity, seat->getPlayer());
        exportHeadersToPacket(serverNotification->mPacket);
        exportToPacket(serverNotification->mPacket, seat);
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

void RenderedMovableEntity::fireRemoveEntity(Seat* seat)
{
    ServerNotification *serverNotification = new ServerNotification(
        ServerNotificationType::removeEntity, seat->getPlayer());
    const std::string& name = getName();
    GameEntityType type = getObjectType();
    serverNotification->mPacket << type;
    serverNotification->mPacket << name;
    ODServer::getSingleton().queueServerNotification(serverNotification);
}

std::string RenderedMovableEntity::getRenderedMovableEntityStreamFormat()
{
    std::string format = MovableGameEntity::getMovableGameEntityStreamFormat();
    if(!format.empty())
        format += "\t";

    format += "opacity\trotationAngle";

    return format;
}

std::vector<Tile*> RenderedMovableEntity::getCoveredTiles()
{
    std::vector<Tile*> tempVector;
    tempVector.push_back(getPositionTile());
    return tempVector;
}

Tile* RenderedMovableEntity::getCoveredTile(int index)
{
    if(index > 0)
    {
        OD_LOG_ERR("name=" + getName() + ", index=" + Helper::toString(index));
        return nullptr;
    }

    return getPositionTile();
}

uint32_t RenderedMovableEntity::numCoveredTiles() const
{
    if(getPositionTile() == nullptr)
        return 0;

    return 1;
}

void RenderedMovableEntity::exportToPacket(ODPacket& os, const Seat* seat) const
{
    MovableGameEntity::exportToPacket(os, seat);
    os << mOpacity;
    os << mRotationAngle;
    os << mHideCoveredTile;
}

void RenderedMovableEntity::importFromPacket(ODPacket& is)
{
    MovableGameEntity::importFromPacket(is);
    OD_ASSERT_TRUE(is >> mOpacity);
    OD_ASSERT_TRUE(is >> mRotationAngle);
    OD_ASSERT_TRUE(is >> mHideCoveredTile);
}

void RenderedMovableEntity::exportToStream(std::ostream& os) const
{
    MovableGameEntity::exportToStream(os);
    os << mOpacity << "\t";
    os << mRotationAngle << "\t";
}

bool RenderedMovableEntity::importFromStream(std::istream& is)
{
    if(!MovableGameEntity::importFromStream(is))
        return false;
    if(!(is >> mOpacity))
        return false;
    if(!(is >> mRotationAngle))
        return false;

    return true;
}
