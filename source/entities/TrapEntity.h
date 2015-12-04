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

#ifndef TRAPENTITY_H
#define TRAPENTITY_H

#include "entities/PersistentObject.h"

#include <string>
#include <iosfwd>

class Seat;
class ODPacket;

enum class TrapEntityType
{
    trapEntity,
    doorEntity
};

ODPacket& operator<<(ODPacket& os, const TrapEntityType& type);
ODPacket& operator>>(ODPacket& is, TrapEntityType& type);

class TrapEntity: public PersistentObject
{
public:
    TrapEntity(GameMap* gameMap, bool isOnServerMap, const std::string& buildingName, const std::string& meshName,
        Tile* tile, Ogre::Real rotationAngle, bool hideCoveredTile, float opacity = 1.0f);
    TrapEntity(GameMap* gameMap, bool isOnServerMap);

    virtual GameEntityType getObjectType() const override;

    virtual TrapEntityType getTrapEntityType() const
    { return TrapEntityType::trapEntity; }

    virtual bool isVisibleForSeat(Seat* seat) override;

    void seatSawTriggering(Seat* seat);
    void notifySeatsWithVision(const std::vector<Seat*>& seats) override;

    static TrapEntity* getTrapEntityFromPacket(GameMap* gameMap, ODPacket& is);

protected:
    virtual void exportHeadersToPacket(ODPacket& os) const override;

private:
    //! List of all the seats (including owning seat) that have vision on this trap (for enemy seats, that means that
    //! they saw it trigger)
    std::vector<Seat*> mSeatsNotHidden;
};

#endif // TRAPENTITY_H
