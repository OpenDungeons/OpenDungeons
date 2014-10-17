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

#include "MissileBoulder.h"
#include "ODPacket.h"
#include "GameMap.h"
#include "Random.h"
#include "LogManager.h"

#include <iostream>

MissileBoulder::MissileBoulder(GameMap* gameMap, Seat* seat, const std::string& senderName, const std::string& meshName,
        const Ogre::Vector3& direction, double damage) :
    MissileObject(gameMap, seat, senderName, meshName, direction, true),
    mDamage(damage),
    nbHits(0)
{
}

MissileBoulder::MissileBoulder(GameMap* gameMap) :
    MissileObject(gameMap),
    mDamage(0.0),
    nbHits(0)
{
}

bool MissileBoulder::hitCreature(GameEntity* entity)
{
    std::vector<Tile*> tiles = entity->getCoveredTiles();
    if(tiles.empty())
        return true;

    Tile* hitTile = tiles[0];
    entity->takeDamage(this, mDamage, hitTile);
    ++nbHits;
    if(Random::Uint(0, 10 - nbHits) <= 0)
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
    MissileBoulder* obj = new MissileBoulder(gameMap);
    return obj;
}

MissileBoulder* MissileBoulder::getMissileBoulderFromPacket(GameMap* gameMap, ODPacket& packet)
{
    MissileBoulder* obj = new MissileBoulder(gameMap);
    return obj;
}
