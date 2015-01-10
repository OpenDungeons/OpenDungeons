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

#ifndef MISSILEOBJECT_H
#define MISSILEOBJECT_H

#include "entities/RenderedMovableEntity.h"

#include <string>
#include <istream>
#include <ostream>

class Creature;
class Room;
class GameMap;
class Tile;
class ODPacket;

class MissileObject: public RenderedMovableEntity
{
public:
    enum MissileType
    {
        oneHit,
        boulder
    };
    MissileObject(GameMap* gameMap, Seat* seat, const std::string& senderName,
        const std::string& meshName, const Ogre::Vector3& direction, bool damageAllies);
    MissileObject(GameMap* gameMap);

    virtual void doUpkeep();

    /*! brief Function called when the missile hits a wall. If it returns true, the missile direction
     * will be set to nextDirection.
     * If it returns false, the missile will be destroyed
     */
    virtual bool wallHitNextDirection(const Ogre::Vector3& actDirection, Tile* tile, Ogre::Vector3& nextDirection)
    { return false; }

    /*! brief Function called when the missile hits a creature. If it returns true, the missile continues
     * If it returns false, the missile will be destroyed
     */
    virtual bool hitCreature(GameEntity* entity)
    { return false; }

    virtual RenderedMovableEntityType getRenderedMovableEntityType()
    { return RenderedMovableEntityType::missileObject; }

    virtual MissileType getMissileType() = 0;

    virtual void exportHeadersToStream(std::ostream& os);
    virtual void exportHeadersToPacket(ODPacket& os);

    static MissileObject* getMissileObjectFromStream(GameMap* gameMap, std::istream& is);
    static MissileObject* getMissileObjectFromPacket(GameMap* gameMap, ODPacket& is);

    friend ODPacket& operator<<(ODPacket& os, const MissileObject::MissileType& rot);
    friend ODPacket& operator>>(ODPacket& is, MissileObject::MissileType& rot);
    friend std::ostream& operator<<(std::ostream& os, const MissileObject::MissileType& rot);
    friend std::istream& operator>>(std::istream& is, MissileObject::MissileType& rot);
private:
    bool computeDestination(const Ogre::Vector3& position, double moveDist, const Ogre::Vector3& direction,
        Ogre::Vector3& destination, std::list<Tile*>& tiles);
    Ogre::Vector3 mDirection;
    bool mIsMissileAlive;
    bool mDamageAllies;
};

#endif // MISSILEOBJECT_H
