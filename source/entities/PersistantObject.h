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

#ifndef PERSISTANTOBJECT_H
#define PERSISTANTOBJECT_H

#include "entities/RenderedMovableEntity.h"

#include <string>
#include <istream>
#include <ostream>

class Seat;
class ODPacket;

/*! PersistantObject is a RenderedMovableEntity that stays displayed when vision is lost (however, the object is not refreshed for clients
 *  without vision ie animation changes or remove from gamemap). When a PersistantObject is removed from the server gamemap, it will stay
 *  displayed on clients until they get vision on the associated tile.
 *  There are 2 problems with PersistantObject:
 *  - Their lifetime on client side can be higher than on client because he should be notified only when he gets vision on the associated
 *    tile. To make it work, on server side, we will send the list of PersistantObject on tiles when they get refreshed. This way, it will
 *    be easy to remove PersistantObject that got removed from server map.
 *  - Because they are displayed but not refreshed when a player looses vision, they might be in a different state on client side
 *    than on server side when a client gets vision again. To make it work, we will remove the PersistantObject from the client and
 *    recreate it when vision is gained again.
*/
class PersistantObject: public RenderedMovableEntity
{
public:
    PersistantObject(GameMap* gameMap, const std::string& buildingName, const std::string& meshName,
        Tile* tile, Ogre::Real rotationAngle, bool hideCoveredTile, float opacity = 1.0f);
    PersistantObject(GameMap* gameMap);

    virtual RenderedMovableEntityType getRenderedMovableEntityType()
    { return RenderedMovableEntityType::persistantObject; }

    //! This has nothing to do with vision. It is just to decide if the tile where the persistant object is should notify it
    //! or not. That would allow for hidden traps, for example, not to send their presence to the clients until they triggered.
    virtual bool isVisibleForSeat(Seat* seat)
    { return true; }

    virtual void notifyAddedOnGamemap();
    virtual void notifyRemovedFromGamemap();

    void notifySeatsWithVision(const std::vector<Seat*>& seats);
    //! We don't want to the remove entity event to be fired when the PersistantObject is removed from the server gamemap
    void fireRemoveEntityToSeatsWithVision()
    {}

    virtual void exportToPacket(ODPacket& os) const;
    virtual void importFromPacket(ODPacket& is);

    static PersistantObject* getPersistantObjectFromPacket(GameMap* gameMap, ODPacket& is);

private:
    Tile* mTile;
    std::vector<Seat*> mSeatsAlreadyNotifiedOnce;
};

#endif // PERSISTANTOBJECT_H
