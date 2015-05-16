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

#ifndef DOORENTITY_H
#define DOORENTITY_H

#include "entities/TrapEntity.h"

#include <string>

class Seat;
class ODPacket;

class DoorEntity: public TrapEntity
{
public:
    DoorEntity(GameMap* gameMap, Seat* seat, const std::string& buildingName, const std::string& meshName,
        Tile* tile, Ogre::Real rotationAngle, bool hideCoveredTile, float opacity,
        const std::string& initialAnimationState, bool initialAnimationLoop);
    DoorEntity(GameMap* gameMap);

    virtual TrapEntityType getTrapEntityType() const override
    { return TrapEntityType::doorEntity; }

    bool canSlap(Seat* seat);
    void slap();

    static DoorEntity* getDoorEntityFromPacket(GameMap* gameMap, ODPacket& is);
    virtual void exportToPacket(ODPacket& os) const override;
    virtual void importFromPacket(ODPacket& is) override;
};

#endif // DOORENTITY_H
