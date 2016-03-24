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

#ifndef DOORENTITY_H
#define DOORENTITY_H

#include "entities/TrapEntity.h"

#include <string>

class Building;
class Seat;
class ODPacket;

class DoorEntity: public TrapEntity, public GameEntityListener
{
public:
    DoorEntity(GameMap* gameMap, Building& building, const std::string& meshName,
        Tile* tile, Ogre::Real rotationAngle, bool hideCoveredTile, float opacity,
        const std::string& initialAnimationState, bool initialAnimationLoop);
    DoorEntity(GameMap* gameMap);

    virtual ~DoorEntity();

    virtual TrapEntityType getTrapEntityType() const override
    { return TrapEntityType::doorEntity; }

    bool canSlap(Seat* seat);
    void slap();

    std::string getListenerName() const override;
    bool notifyDead(GameEntity* entity) override;
    bool notifyRemovedFromGameMap(GameEntity* entity) override;
    bool notifyPickedUp(GameEntity* entity) override;
    bool notifyDropped(GameEntity* entity) override;

    static DoorEntity* getDoorEntityFromPacket(GameMap* gameMap, ODPacket& is);
protected:
    virtual void exportToPacket(ODPacket& os, const Seat* seat) const override;
    virtual void importFromPacket(ODPacket& is) override;
private:
    Building* mBuilding;
};

#endif // DOORENTITY_H
