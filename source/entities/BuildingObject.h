/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#ifndef BUILDINGOBJECT_H
#define BUILDINGOBJECT_H

#include "entities/RenderedMovableEntity.h"

#include <string>
#include <iosfwd>

class Building;
class Creature;
class Room;
class GameMap;
class Tile;
class ODPacket;

class BuildingObject: public RenderedMovableEntity
{
public:
    BuildingObject(GameMap* gameMap, Building& building, const std::string& meshName, Tile* targetTile,
        Ogre::Real x, Ogre::Real y, Ogre::Real z, Ogre::Real rotationAngle, const Ogre::Vector3& scale,
        bool hideCoveredTile, float opacity = 1.0f, const std::string& initialAnimationState = "", bool initialAnimationLoop = true);
    BuildingObject(GameMap* gameMap, Building& building, const std::string& meshName,
        Tile& targetTile, Ogre::Real rotationAngle, const Ogre::Vector3& scale, bool hideCoveredTile, float opacity = 1.0f,
        const std::string& initialAnimationState = "", bool initialAnimationLoop = true);
    BuildingObject(GameMap* gameMap);

    virtual GameEntityType getObjectType() const override;

    virtual const Ogre::Vector3& getScale() const override
    { return mScale; }

    void doUpkeep() override;

    void addParticleEffect(const std::string& effectScript, uint32_t nbTurns);

    void fireRefresh();

    static BuildingObject* getBuildingObjectFromPacket(GameMap* gameMap, ODPacket& is);

protected:
    virtual void exportToPacket(ODPacket& os, const Seat* seat) const override;
    virtual void importFromPacket(ODPacket& is) override;
    virtual void exportToStream(std::ostream& os) const override;
    virtual bool importFromStream(std::istream& is) override;

private:
    Ogre::Vector3 mScale;
};

#endif // BUILDINGOBJECT_H
