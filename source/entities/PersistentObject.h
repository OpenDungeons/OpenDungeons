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

#ifndef PERSISTENTOBJECT_H
#define PERSISTENTOBJECT_H

#include "entities/BuildingObject.h"

#include <string>
#include <iosfwd>

class Building;
class Seat;
class ODPacket;

/*! PersistentObject is a BuildingObject that stays displayed when vision is
 *  lost (however, the object is not refreshed for clients without vision ie animation
 *  changes). Note that Persistent objects will be removed from the clients if they are
 *  removed from the server gamemap. That means that buildings that use them and want them
 *  to be displayed until a client gains vision again should take this into account and
 *  not add them to building objects as they would be removed if the room is destroyed.
 *  Because PersistentObjects are displayed but not refreshed when a player looses vision,
 *  they might be in a different state on client side than on server side when a client gets
 *  vision again. To make sure they are synchronized, when a player re-gains vision on a
 *  PersistentObject he had already seen, we will remove it from the player gamemap and
 *  recreate it.
*/
class PersistentObject: public BuildingObject
{
public:
    PersistentObject(GameMap* gameMap, Building& building, const std::string& meshName,
        Tile* tile, Ogre::Real rotationAngle, bool hideCoveredTile, float opacity = 1.0f,
        const std::string& initialAnimationState = "", bool initialAnimationLoop = true);
    PersistentObject(GameMap* gameMap);

    virtual GameEntityType getObjectType() const override;

    //! This has nothing to do with vision. It is just to decide if the tile where the persistent object is should notify it
    //! or not. That would allow for hidden traps, for example, not to send their presence to the clients until they triggered.
    virtual bool isVisibleForSeat(Seat* seat)
    { return true; }

    virtual void notifySeatsWithVision(const std::vector<Seat*>& seats) override;
    virtual void fireRemoveEntityToSeatsWithVision() override;

    virtual bool notifyRemoveAsked() override;

    const std::vector<Seat*>& getSeatsAlreadyNotifiedOnce() const
    { return mSeatsAlreadyNotifiedOnce; }

    static PersistentObject* getPersistentObjectFromPacket(GameMap* gameMap, ODPacket& is);

protected:
    virtual void exportToPacket(ODPacket& os, const Seat* seat) const override;
    virtual void importFromPacket(ODPacket& is) override;

    std::vector<Seat*> mSeatsAlreadyNotifiedOnce;

private:
    Tile* mTile;

    //! If true, the PersistentObject will notify the clients about vision
    //! If false, the PersistentObject will only register clients that lost vision
    //! and, once every client will have seen the PersistentObject been removed
    //! it will be removed from gamemap and will allow its containing building to
    //! get removed
    bool mIsWorking;
};

#endif // PERSISTENTOBJECT_H
