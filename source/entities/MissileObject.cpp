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

#include "entities/MissileObject.h"

#include "entities/MissileBoulder.h"
#include "entities/MissileOneHit.h"

#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "utils/LogManager.h"

#include <iostream>

MissileObject::MissileObject(GameMap* gameMap, Seat* seat, const std::string& senderName,
        const std::string& meshName, const Ogre::Vector3& direction, bool damageAllies) :
    RenderedMovableEntity(gameMap, senderName, meshName, 0.0f),
    mDirection(direction),
    mIsMissileAlive(true),
    mDamageAllies(damageAllies)
{
    setSeat(seat);
}

MissileObject::MissileObject(GameMap* gameMap) :
    RenderedMovableEntity(gameMap),
    mDirection(Ogre::Vector3::ZERO),
    mIsMissileAlive(true),
    mDamageAllies(false)
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
            getGameMap()->removeRenderedMovableEntity(this);
            deleteYourself();
        }
        return;
    }

    Tile* tile = getPositionTile();
    OD_ASSERT_TRUE_MSG(tile != nullptr, "entityName=" + getName());
    if(tile == nullptr)
        return;

    // We check if a creature is in our way. We start by taking the tile we will be on
    Ogre::Vector3 position = getPosition();
    double moveDist = getMoveSpeed();
    Ogre::Vector3 destination = position + (moveDist * mDirection);

    std::list<Tile*> tiles = getGameMap()->tilesBetween(static_cast<int>(position.x),
        static_cast<int>(position.y), static_cast<int>(destination.x), static_cast<int>(destination.y));

    OD_ASSERT_TRUE(!tiles.empty());
    if(tiles.empty())
    {
        getGameMap()->removeRenderedMovableEntity(this);
        deleteYourself();
        return;
    }

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
                destination -= moveDist * mDirection;
                break;
            }
            else
            {
                position.x = static_cast<Ogre::Real>(lastTile->getX());
                position.y = static_cast<Ogre::Real>(lastTile->getY());
                addDestination(position.x, position.y, position.z);
                // We compute next position
                mDirection = nextDirection;
                destination = position + (moveDist * mDirection);
                tiles = getGameMap()->tilesBetween(static_cast<int>(position.x),
                    static_cast<int>(position.y), static_cast<int>(destination.x), static_cast<int>(destination.y));

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

    addDestination(destination.x, destination.y, destination.z);
}

void MissileObject::exportHeadersToStream(std::ostream& os)
{
    RenderedMovableEntity::exportHeadersToStream(os);
    os << getMissileType() << "\t";
}

void MissileObject::exportHeadersToPacket(ODPacket& os)
{
    RenderedMovableEntity::exportHeadersToPacket(os);
    os << getMissileType();
}

MissileObject* MissileObject::getMissileObjectFromStream(GameMap* gameMap, std::istream& is)
{
    MissileObject* obj = nullptr;
    MissileType type;
    OD_ASSERT_TRUE(is >> type);
    switch(type)
    {
        case MissileType::oneHit:
        {
            obj = MissileOneHit::getMissileOneHitFromStream(gameMap, is);
            break;
        }
        case MissileType::boulder:
        {
            obj = MissileBoulder::getMissileBoulderFromStream(gameMap, is);
            break;
        }
        default:
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(type)));
            break;
    }
    return obj;
}

MissileObject* MissileObject::getMissileObjectFromPacket(GameMap* gameMap, ODPacket& is)
{
    MissileObject* obj = nullptr;
    MissileType type;
    OD_ASSERT_TRUE(is >> type);
    switch(type)
    {
        case MissileType::oneHit:
        {
            obj = MissileOneHit::getMissileOneHitFromPacket(gameMap, is);
            break;
        }
        case MissileType::boulder:
        {
            obj = MissileBoulder::getMissileBoulderFromPacket(gameMap, is);
            break;
        }
        default:
            OD_ASSERT_TRUE_MSG(false, "Unknown enum value : " + Ogre::StringConverter::toString(
                static_cast<int>(type)));
            break;
    }
    return obj;
}

ODPacket& operator<<(ODPacket& os, const MissileObject::MissileType& type)
{
    uint32_t intType = static_cast<MissileObject::MissileType>(type);
    os << intType;
    return os;
}

ODPacket& operator>>(ODPacket& is, MissileObject::MissileType& type)
{
    uint32_t intType;
    is >> intType;
    type = static_cast<MissileObject::MissileType>(intType);
    return is;
}

std::ostream& operator<<(std::ostream& os, const MissileObject::MissileType& type)
{
    uint32_t intType = static_cast<MissileObject::MissileType>(type);
    os << intType;
    return os;
}

std::istream& operator>>(std::istream& is, MissileObject::MissileType& type)
{
    uint32_t intType;
    is >> intType;
    type = static_cast<MissileObject::MissileType>(intType);
    return is;
}
