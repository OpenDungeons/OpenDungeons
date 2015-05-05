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

#ifndef MISSILEONEHIT_H
#define MISSILEONEHIT_H

#include "entities/MissileObject.h"

#include <string>
#include <iosfwd>

class Creature;
class Room;
class GameMap;
class Tile;
class ODPacket;

class MissileOneHit: public MissileObject
{
public:
    MissileOneHit(GameMap* gameMap, Seat* seat, const std::string& senderName, const std::string& meshName,
        const std::string& particleScript, const Ogre::Vector3& direction, double physicalDamage, double magicalDamage,
        bool damageAllies);
    MissileOneHit(GameMap* gameMap);

    virtual MissileObjectType getMissileType() const override
    { return MissileObjectType::oneHit; }

    virtual bool hitCreature(GameEntity* entity) override;

    void exportToStream(std::ostream& os) const override;
    void importFromStream(std::istream& is) override;
    void exportToPacket(ODPacket& os) const override;
    void importFromPacket(ODPacket& is) override;

    static MissileOneHit* getMissileOneHitFromStream(GameMap* gameMap, std::istream& is);
    static MissileOneHit* getMissileOneHitFromPacket(GameMap* gameMap, ODPacket& packet);
private:
    double mPhysicalDamage;
    double mMagicalDamage;
};

#endif // MISSILEONEHIT_H
