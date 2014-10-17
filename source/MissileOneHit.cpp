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

#include "MissileOneHit.h"
#include "ODPacket.h"
#include "GameMap.h"
#include "Random.h"
#include "LogManager.h"

#include <iostream>

MissileOneHit::MissileOneHit(GameMap* gameMap, Seat* seat, const std::string& senderName, const std::string& meshName,
        const Ogre::Vector3& direction, double damage, bool damageAllies) :
    MissileObject(gameMap, seat, senderName, meshName, direction, damageAllies),
    mDamage(damage)
{
}

MissileOneHit::MissileOneHit(GameMap* gameMap) :
    MissileObject(gameMap),
    mDamage(0.0)
{
}

bool MissileOneHit::hitCreature(GameEntity* entity)
{
    std::vector<Tile*> tiles = entity->getCoveredTiles();
    if(tiles.empty())
        return true;

    Tile* hitTile = tiles[0];
    entity->takeDamage(this, mDamage, hitTile);
    return false;
}

MissileOneHit* MissileOneHit::getMissileOneHitFromStream(GameMap* gameMap, std::istream& is)
{
    MissileOneHit* obj = new MissileOneHit(gameMap);
    return obj;
}

MissileOneHit* MissileOneHit::getMissileOneHitFromPacket(GameMap* gameMap, ODPacket& packet)
{
    MissileOneHit* obj = new MissileOneHit(gameMap);
    return obj;
}
