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

#include "entities/MissileBoulder.h"

#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

#include <iostream>

MissileBoulder::MissileBoulder(GameMap* gameMap, bool isOnServerMap, Seat* seat, const std::string& senderName, const std::string& meshName,
        const Ogre::Vector3& direction, double speed, double damage, GameEntity* entityTarget) :
    MissileObject(gameMap, isOnServerMap, seat, senderName, meshName, direction, speed, entityTarget, true, false),
    mDamage(damage),
    mNbHits(0)
{
}

MissileBoulder::MissileBoulder(GameMap* gameMap, bool isOnServerMap) :
    MissileObject(gameMap, isOnServerMap),
    mDamage(0.0),
    mNbHits(0)
{
}

bool MissileBoulder::hitCreature(Tile* tile, GameEntity* entity)
{
    entity->takeDamage(this, mDamage, 0.0, 0.0, tile, false, false, false, false);
    ++mNbHits;
    if(Random::Uint(0, 10 - mNbHits) <= 0)
        return false;

    return true;
}

bool MissileBoulder::wallHitNextDirection(const Ogre::Vector3& actDirection, Tile* tile, Ogre::Vector3& nextDirection)
{
    // When we hit a wall, we might break
    if(Random::Uint(1, 2) == 1)
        return false;

    if(Random::Uint(1, 2) == 1)
    {
        nextDirection.x = actDirection.y;
        nextDirection.y = actDirection.x;
        nextDirection.z = actDirection.z;
    }
    else
    {
        nextDirection.x = -actDirection.y;
        nextDirection.y = -actDirection.x;
        nextDirection.z = actDirection.z;
    }
    return true;
}

MissileBoulder* MissileBoulder::getMissileBoulderFromStream(GameMap* gameMap, std::istream& is)
{
    MissileBoulder* obj = new MissileBoulder(gameMap, true);
    obj->importFromStream(is);
    return obj;
}

MissileBoulder* MissileBoulder::getMissileBoulderFromPacket(GameMap* gameMap, ODPacket& is)
{
    MissileBoulder* obj = new MissileBoulder(gameMap, false);
    obj->importFromPacket(is);
    return obj;
}

void MissileBoulder::exportToStream(std::ostream& os) const
{
    MissileObject::exportToStream(os);
    os << mDamage << "\t";
    os << mNbHits << "\t";
}

bool MissileBoulder::importFromStream(std::istream& is)
{
    if(!MissileObject::importFromStream(is))
        return false;
    if(!(is >> mDamage))
        return false;
    if(!(is >> mNbHits))
        return false;

    return true;
}
