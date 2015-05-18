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

const Ogre::Vector3 SCALE(0.7,0.7,0.7);

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

void RenderedMovableEntity::addToGameMap()
{
    getGameMap()->addRenderedMovableEntity(this);
    setIsOnMap(true);
    getGameMap()->addAnimatedObject(this);

    if(!getGameMap()->isServerGameMap())
        return;

    getGameMap()->addActiveObject(this);
}

void RenderedMovableEntity::removeFromGameMap()
{
    getGameMap()->removeRenderedMovableEntity(this);
    setIsOnMap(false);
    getGameMap()->removeAnimatedObject(this);

    Tile* posTile = getPositionTile();
    if(posTile != nullptr)
        posTile->removeEntity(this);

    if(!getGameMap()->isServerGameMap())
        return;

    fireRemoveEntityToSeatsWithVision();

    getGameMap()->removeActiveObject(this);
}

void RenderedMovableEntity::setMeshOpacity(float opacity)
{
    if (opacity < 0.0f || opacity > 1.0f)
        return;

    mOpacity = opacity;

    if(getGameMap()->isServerGameMap())
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
            ServerNotificationType::addEntity, seat->getPlayer());
        exportHeadersToPacket(serverNotification.mPacket);
        exportToPacket(serverNotification.mPacket);
        ODServer::getSingleton().sendAsyncMsg(serverNotification);
    }
    else
    {
        ServerNotification* serverNotification = new ServerNotification(
            ServerNotificationType::addEntity, seat->getPlayer());
        exportHeadersToPacket(serverNotification->mPacket);
        exportToPacket(serverNotification->mPacket);
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
    OD_ASSERT_TRUE_MSG(index == 0, "name=" + getName()
        + ", index=" + Ogre::StringConverter::toString(index));

    if(index > 0)
        return nullptr;

    return getPositionTile();
}

uint32_t RenderedMovableEntity::numCoveredTiles()
{
    if(getPositionTile() == nullptr)
        return 0;

    return 1;
}

void RenderedMovableEntity::exportToPacket(ODPacket& os) const
{
    MovableGameEntity::exportToPacket(os);
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

void RenderedMovableEntity::importFromStream(std::istream& is)
{
    MovableGameEntity::importFromStream(is);
    OD_ASSERT_TRUE(is >> mOpacity);
    OD_ASSERT_TRUE(is >> mRotationAngle);
}
