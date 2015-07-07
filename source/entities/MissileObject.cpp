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

#include "entities/MissileObject.h"

#include "entities/Building.h"
#include "entities/MissileBoulder.h"
#include "entities/MissileOneHit.h"
#include "entities/Tile.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <iostream>

MissileObject::MissileObject(GameMap* gameMap, bool isOnServerMap, Seat* seat, const std::string& senderName, const std::string& meshName,
        const Ogre::Vector3& direction, double speed, Tile* tileBuildingTarget, bool damageAllies) :
    RenderedMovableEntity(gameMap, isOnServerMap, senderName, meshName, 0.0f, false),
    mDirection(direction),
    mIsMissileAlive(true),
    mTileBuildingTarget(tileBuildingTarget),
    mDamageAllies(damageAllies),
    mSpeed(speed)
{
    setSeat(seat);
}

MissileObject::MissileObject(GameMap* gameMap, bool isOnServerMap) :
    RenderedMovableEntity(gameMap, isOnServerMap),
    mDirection(Ogre::Vector3::ZERO),
    mIsMissileAlive(true),
    mTileBuildingTarget(nullptr),
    mDamageAllies(false),
    mSpeed(1.0)
{
}

void MissileObject::doUpkeep()
{
    if(!getIsOnMap())
        return;

    if(!mIsMissileAlive)
    {
        if(!isMoving())
        {
            removeFromGameMap();
            deleteYourself();
        }
        return;
    }

    Tile* tile = getPositionTile();
    if(tile == nullptr)
    {
        OD_LOG_ERR("entityName=" + getName());
        removeFromGameMap();
        deleteYourself();
        return;
    }

    // We check if a creature is in our way. We start by taking the tile we will be on
    Ogre::Vector3 position = getPosition();
    double moveDist = getMoveSpeed();
    Ogre::Vector3 destination;
    std::list<Tile*> tiles;
    mIsMissileAlive = computeDestination(position, moveDist, mDirection, destination, tiles);

    std::vector<Ogre::Vector3> path;
    Tile* lastTile = nullptr;
    while(!tiles.empty() && mIsMissileAlive)
    {
        Tile* tmpTile = tiles.front();
        tiles.pop_front();

        if(tmpTile->getFullness() > 0.0)
        {
            Ogre::Vector3 nextDirection;
            mIsMissileAlive = wallHitNextDirection(mDirection, lastTile, nextDirection);
            if(!mIsMissileAlive)
            {
                destination.x = static_cast<Ogre::Real>(lastTile->getX());
                destination.y = static_cast<Ogre::Real>(lastTile->getY());
                break;
            }
            else
            {
                position.x = static_cast<Ogre::Real>(lastTile->getX());
                position.y = static_cast<Ogre::Real>(lastTile->getY());
                path.push_back(position);
                // We compute next position
                mDirection = nextDirection;
                mIsMissileAlive = computeDestination(position, moveDist, mDirection, destination, tiles);
                continue;
            }
        }

        if(lastTile != nullptr)
        {
            Ogre::Vector3 lastPos;
            lastPos.x = static_cast<Ogre::Real>(lastTile->getX());
            lastPos.y = static_cast<Ogre::Real>(lastTile->getY());
            lastPos.z = position.z;
            Ogre::Vector3 curPos;
            curPos.x = static_cast<Ogre::Real>(tmpTile->getX());
            curPos.y = static_cast<Ogre::Real>(tmpTile->getY());
            curPos.z = position.z;
            moveDist -= lastPos.distance(curPos);
        }
        lastTile = tmpTile;

        // If we are aiming a building, we check if we hit
        if((mTileBuildingTarget != nullptr) &&
           (mTileBuildingTarget == tmpTile))
        {
            // We hit
            Building* building = mTileBuildingTarget->getCoveringBuilding();
            // Note that building may be null here is the building got destroyed before the missile gets there
            if((building != nullptr) &&
               (building->isAttackable(mTileBuildingTarget, mSeat)) &&
               (!building->getSeat()->isAlliedSeat(mSeat)))
            {
                hitTargetBuilding(mTileBuildingTarget, building);
                mIsMissileAlive = false;
                destination.x = static_cast<Ogre::Real>(tmpTile->getX());
                destination.y = static_cast<Ogre::Real>(tmpTile->getY());
            }
        }

        std::vector<Tile*> tileVector;
        tileVector.push_back(tmpTile);
        std::vector<GameEntity*> enemyCreatures = getGameMap()->getVisibleCreatures(tileVector, getSeat(), true);
        for(std::vector<GameEntity*>::iterator it = enemyCreatures.begin(); it != enemyCreatures.end(); ++it)
        {
            GameEntity* creature = *it;
            if(!hitCreature(creature))
            {
                destination -= moveDist * mDirection;
                mIsMissileAlive = false;
                break;
            }
        }

        if(!mDamageAllies || !mIsMissileAlive)
            continue;

        std::vector<GameEntity*> alliedCreatures = getGameMap()->getVisibleCreatures(tileVector, getSeat(), false);
        for(std::vector<GameEntity*>::iterator it = alliedCreatures.begin(); it != alliedCreatures.end(); ++it)
        {
            GameEntity* creature = *it;
            if(!hitCreature(creature))
            {
                destination -= moveDist * mDirection;
                mIsMissileAlive = false;
                break;
            }
        }
    }

    path.push_back(destination);
    setWalkPath(EntityAnimation::idle_anim, EntityAnimation::idle_anim, true, path);
}

bool MissileObject::computeDestination(const Ogre::Vector3& position, double moveDist, const Ogre::Vector3& direction,
        Ogre::Vector3& destination, std::list<Tile*>& tiles)
{
    destination = position + (moveDist * direction);
    tiles = getGameMap()->tilesBetween(Helper::round(position.x),
        Helper::round(position.y), Helper::round(destination.x), Helper::round(destination.y));
    if(tiles.empty())
    {
        OD_LOG_ERR("missile=" + getName() + " has unexpected empty tiles destination");
        return false;
    }

    // If we get out of the map, we take the last tile as the destination
    if((direction.x > 0.0 && destination.x > static_cast<Ogre::Real>(getGameMap()->getMapSizeX() - 1)) ||
       (direction.x < 0.0 && destination.x < 0.0) ||
       (direction.y > 0 && destination.y > static_cast<Ogre::Real>(getGameMap()->getMapSizeY() - 1)) ||
       (direction.y < 0 && destination.y < 0))
    {
        Tile* lastTile = tiles.back();
        destination.x = static_cast<Ogre::Real>(lastTile->getX());
        destination.y = static_cast<Ogre::Real>(lastTile->getY());

        // We are in the last position, we can die
        if(tiles.size() <= 1)
            return false;
    }

    return true;
}

void MissileObject::exportHeadersToStream(std::ostream& os) const
{
    RenderedMovableEntity::exportHeadersToStream(os);
    os << getMissileType() << "\t";
}

void MissileObject::exportHeadersToPacket(ODPacket& os) const
{
    RenderedMovableEntity::exportHeadersToPacket(os);
    os << getMissileType();
}

void MissileObject::exportToPacket(ODPacket& os) const
{
    RenderedMovableEntity::exportToPacket(os);
    os << mSpeed;
}

void MissileObject::importFromPacket(ODPacket& is)
{
    RenderedMovableEntity::importFromPacket(is);
    OD_ASSERT_TRUE(is >> mSpeed);
}

void MissileObject::exportToStream(std::ostream& os) const
{
    RenderedMovableEntity::exportToStream(os);
    os << mDirection.x << "\t" << mDirection.y << "\t" << mDirection.z << "\t";
    os << mIsMissileAlive << "\t";
    os << mDamageAllies << "\t";
    os << mSpeed << "\t";
}

void MissileObject::importFromStream(std::istream& is)
{
    RenderedMovableEntity::importFromStream(is);
    OD_ASSERT_TRUE(is >> mDirection.x >> mDirection.y >> mDirection.z);
    OD_ASSERT_TRUE(is >> mIsMissileAlive);
    OD_ASSERT_TRUE(is >> mDamageAllies);
    OD_ASSERT_TRUE(is >> mSpeed);
}

std::string MissileObject::getMissileObjectStreamFormat()
{
    std::string format = RenderedMovableEntity::getRenderedMovableEntityStreamFormat();
    if(!format.empty())
        format += "\t";

    format += "directionX\tdirectionY\tdirectionZ\tmissileAlive\tdamageAllies\tspeed\toptionalData";

    return "missileType\t" + format;
}

MissileObject* MissileObject::getMissileObjectFromStream(GameMap* gameMap, std::istream& is)
{
    MissileObject* obj = nullptr;
    MissileObjectType type;
    OD_ASSERT_TRUE(is >> type);
    switch(type)
    {
        case MissileObjectType::oneHit:
        {
            obj = MissileOneHit::getMissileOneHitFromStream(gameMap, is);
            break;
        }
        case MissileObjectType::boulder:
        {
            obj = MissileBoulder::getMissileBoulderFromStream(gameMap, is);
            break;
        }
        default:
            OD_LOG_ERR("Unknown enum value : " + Helper::toString(
                static_cast<int>(type)));
            break;
    }
    return obj;
}

MissileObject* MissileObject::getMissileObjectFromPacket(GameMap* gameMap, ODPacket& is)
{
    MissileObject* obj = nullptr;
    MissileObjectType type;
    OD_ASSERT_TRUE(is >> type);
    switch(type)
    {
        case MissileObjectType::oneHit:
        {
            obj = MissileOneHit::getMissileOneHitFromPacket(gameMap, is);
            break;
        }
        case MissileObjectType::boulder:
        {
            obj = MissileBoulder::getMissileBoulderFromPacket(gameMap, is);
            break;
        }
        default:
            OD_LOG_ERR("Unknown enum value : " + Helper::toString(
                static_cast<int>(type)));
            break;
    }
    return obj;
}

ODPacket& operator<<(ODPacket& os, const MissileObjectType& type)
{
    uint32_t intType = static_cast<uint32_t>(type);
    os << intType;
    return os;
}

ODPacket& operator>>(ODPacket& is, MissileObjectType& type)
{
    uint32_t intType;
    is >> intType;
    type = static_cast<MissileObjectType>(intType);
    return is;
}

std::ostream& operator<<(std::ostream& os, const MissileObjectType& type)
{
    uint32_t intType = static_cast<uint32_t>(type);
    os << intType;
    return os;
}

std::istream& operator>>(std::istream& is, MissileObjectType& type)
{
    uint32_t intType;
    is >> intType;
    type = static_cast<MissileObjectType>(intType);
    return is;
}
