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
#include <iosfwd>

class Building;
class Creature;
class Room;
class GameMap;
class Tile;
class ODPacket;

enum class MissileObjectType
{
    oneHit,
    boulder
};

ODPacket& operator<<(ODPacket& os, const MissileObjectType& rot);
ODPacket& operator>>(ODPacket& is, MissileObjectType& rot);
std::ostream& operator<<(std::ostream& os, const MissileObjectType& rot);
std::istream& operator>>(std::istream& is, MissileObjectType& rot);

//! Class used on client side to display the particle effects on the missile
class MissileParticuleEffectClient : public EntityParticleEffect
{
public:
    MissileParticuleEffectClient(const std::string& name, const std::string& script, uint32_t nbTurnsEffect) :
        EntityParticleEffect(name, script, nbTurnsEffect)
    {}

    virtual EntityParticleEffectType getEntityParticleEffectType() override
    { return EntityParticleEffectType::missile; }
};

class MissileObject: public RenderedMovableEntity
{
public:
    //! If the missile is sent against a building, tileBuildingTarget should contain it. If the tile is reached,
    //! the building will be damaged. If the target is not a building, tileBuildingTarget should be nullptr
    MissileObject(GameMap* gameMap, bool isOnServerMap, Seat* seat, const std::string& senderName, const std::string& meshName,
        const Ogre::Vector3& direction, double speed, Tile* tileBuildingTarget, bool damageAllies);
    MissileObject(GameMap* gameMap, bool isOnServerMap);

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

    /*! brief Function called when the missile hits the target building. After returning, the missile will be destroyed
     */
    virtual void hitTargetBuilding(Tile* tile, Building* target)
    {}

    virtual GameEntityType getObjectType() const override
    { return GameEntityType::missileObject; }

    virtual double getMoveSpeed() const override
    { return mSpeed; }

    virtual MissileObjectType getMissileType() const = 0;

    static std::string getMissileObjectStreamFormat();

    static MissileObject* getMissileObjectFromStream(GameMap* gameMap, std::istream& is);
    static MissileObject* getMissileObjectFromPacket(GameMap* gameMap, ODPacket& is);
protected:
    virtual void exportHeadersToStream(std::ostream& os) const override;
    virtual void exportHeadersToPacket(ODPacket& os) const override;
    void exportToStream(std::ostream& os) const override;
    void importFromStream(std::istream& is) override;
    void exportToPacket(ODPacket& os) const override;
    void importFromPacket(ODPacket& is) override;

private:
    bool computeDestination(const Ogre::Vector3& position, double moveDist, const Ogre::Vector3& direction,
        Ogre::Vector3& destination, std::list<Tile*>& tiles);
    Ogre::Vector3 mDirection;
    bool mIsMissileAlive;
    Tile* mTileBuildingTarget;
    bool mDamageAllies;
    double mSpeed;
};

#endif // MISSILEOBJECT_H
