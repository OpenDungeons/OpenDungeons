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

#ifndef MISSILEBOULDER_H
#define MISSILEBOULDER_H

#include "entities/MissileObject.h"

#include <string>
#include <istream>
#include <ostream>

class Creature;
class Room;
class GameMap;
class Tile;
class ODPacket;

class MissileBoulder: public MissileObject
{
public:
    MissileBoulder(GameMap* gameMap, Seat* seat, const std::string& senderName, const std::string& meshName,
        const Ogre::Vector3& direction, double damage);
    MissileBoulder(GameMap* gameMap);

    virtual MissileType getMissileType()
    { return MissileType::boulder; }

    virtual bool hitCreature(GameEntity* entity);
    virtual bool wallHitNextDirection(const Ogre::Vector3& actDirection, Tile* tile, Ogre::Vector3& nextDirection);

    static MissileBoulder* getMissileBoulderFromStream(GameMap* gameMap, std::istream& is);
    static MissileBoulder* getMissileBoulderFromPacket(GameMap* gameMap, ODPacket& packet);
private:
    double mDamage;
    int mNbHits;
};

#endif // MISSILEBOULDER_H
