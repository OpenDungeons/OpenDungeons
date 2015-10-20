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
    MissileOneHit(GameMap* gameMap, bool isOnServerMap, Seat* seat, const std::string& senderName, const std::string& meshName,
        const std::string& particleScript, const Ogre::Vector3& direction, double speed, double physicalDamage, double magicalDamage,
        double elementDamage, GameEntity* entityTarget, bool damageAllies, bool koEnemyCreature);
    MissileOneHit(GameMap* gameMap, bool isOnServerMap);

    virtual MissileObjectType getMissileType() const override
    { return MissileObjectType::oneHit; }

    virtual bool hitCreature(Tile* tile, GameEntity* entity) override;

    virtual void hitTargetEntity(Tile* tile, GameEntity* entityTarget) override;

    static MissileOneHit* getMissileOneHitFromStream(GameMap* gameMap, std::istream& is);
    static MissileOneHit* getMissileOneHitFromPacket(GameMap* gameMap, ODPacket& is);
protected:
    void exportToStream(std::ostream& os) const override;
    bool importFromStream(std::istream& is) override;
    void exportToPacket(ODPacket& os, const Seat* seat) const override;
    void importFromPacket(ODPacket& is) override;

private:
    double mPhysicalDamage;
    double mMagicalDamage;
    double mElementDamage;
};

#endif // MISSILEONEHIT_H
