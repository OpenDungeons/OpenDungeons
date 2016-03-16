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

#include "entities/BuildingObject.h"

#include "entities/Building.h"
#include "entities/GameEntityType.h"
#include "entities/Tile.h"
#include "network/ODPacket.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "traps/TrapBoulder.h"
#include "traps/TrapCannon.h"
#include "traps/TrapSpike.h"
#include "utils/Random.h"
#include "utils/LogManager.h"

#include <iostream>

BuildingObject::BuildingObject(GameMap* gameMap, Building& building, const std::string& meshName, Tile* targetTile,
        Ogre::Real x, Ogre::Real y, Ogre::Real z, Ogre::Real rotationAngle, const Ogre::Vector3& scale,
        bool hideCoveredTile, float opacity, const std::string& initialAnimationState, bool initialAnimationLoop) :
    RenderedMovableEntity(
        gameMap,
        targetTile == nullptr ? building.getName() : building.getName() + "_" + Tile::displayAsString(targetTile),
        meshName,
        rotationAngle,
        hideCoveredTile,
        opacity),
    mScale(scale)
{
    mPosition = Ogre::Vector3(x, y, z);
    mPrevAnimationState = initialAnimationState;
    mPrevAnimationStateLoop = initialAnimationLoop;
}

BuildingObject::BuildingObject(GameMap* gameMap, Building& building, const std::string& meshName,
        Tile& targetTile, Ogre::Real rotationAngle, const Ogre::Vector3& scale, bool hideCoveredTile, float opacity,
        const std::string& initialAnimationState, bool initialAnimationLoop) :
    RenderedMovableEntity(
        gameMap,
        building.getName() + "_" + Tile::displayAsString(&targetTile),
        meshName,
        rotationAngle,
        hideCoveredTile,
        opacity),
    mScale(scale)
{
    mPosition = Ogre::Vector3(targetTile.getX(), targetTile.getY(), 0);
    mPrevAnimationState = initialAnimationState;
    mPrevAnimationStateLoop = initialAnimationLoop;
}


BuildingObject::BuildingObject(GameMap* gameMap) :
    RenderedMovableEntity(gameMap),
    mScale(1.0f, 1.0f, 1.0f)
{
}

GameEntityType BuildingObject::getObjectType() const
{
    return GameEntityType::buildingObject;
}

void BuildingObject::doUpkeep()
{
    RenderedMovableEntity::doUpkeep();

    for(auto it = mEntityParticleEffects.begin(); it != mEntityParticleEffects.end();)
    {
        EntityParticleEffect* effect = *it;
        if(effect->mNbTurnsEffect < 0)
        {
            ++it;
            continue;
        }

        if(effect->mNbTurnsEffect > 0)
        {
            --effect->mNbTurnsEffect;
            ++it;
            continue;
        }

        it = mEntityParticleEffects.erase(it);
        delete effect;
    }
}

BuildingObject* BuildingObject::getBuildingObjectFromPacket(GameMap* gameMap, ODPacket& is)
{
    BuildingObject* obj = new BuildingObject(gameMap);
    obj->importFromPacket(is);
    return obj;
}

void BuildingObject::exportToPacket(ODPacket& os, const Seat* seat) const
{
    RenderedMovableEntity::exportToPacket(os, seat);
    os << mScale;
}

void BuildingObject::importFromPacket(ODPacket& is)
{
    RenderedMovableEntity::importFromPacket(is);
    OD_ASSERT_TRUE(is >> mScale);
}

void BuildingObject::exportToStream(std::ostream& os) const
{
    RenderedMovableEntity::exportToStream(os);
    os << mScale.x;
    os << mScale.y;
    os << mScale.z;
}

bool BuildingObject::importFromStream(std::istream& is)
{
    if(!RenderedMovableEntity::importFromStream(is))
        return false;
    if(!(is >> mScale.x))
        return false;
    if(!(is >> mScale.y))
        return false;
    if(!(is >> mScale.z))
        return false;

    return true;
}

void BuildingObject::addParticleEffect(const std::string& effectScript, uint32_t nbTurns)
{
    EntityParticleEffect* effect = new EntityParticleEffect(nextParticleSystemsName(), effectScript, nbTurns);
    mEntityParticleEffects.push_back(effect);
}

void BuildingObject::fireRefresh()
{
    for(Seat* seat : mSeatsWithVisionNotified)
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        const std::string& name = getName();
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::entitiesRefresh, seat->getPlayer());
        uint32_t nb = 1;
        GameEntityType entityType = getObjectType();
        serverNotification->mPacket << nb;
        serverNotification->mPacket << entityType;
        serverNotification->mPacket << name;
        exportToPacketForUpdate(serverNotification->mPacket, seat);
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}
