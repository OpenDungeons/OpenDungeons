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

#ifndef ROOMPORTAL_H
#define ROOMPORTAL_H

#include "rooms/Room.h"
#include "rooms/RoomType.h"

class RoomPortal: public Room
{
public:
    RoomPortal(GameMap* gameMap);

    inline uint32_t getNbCreatureMaxIncrease() const
    { return mNbCreatureMaxIncrease; }

    virtual RoomType getType() const override
    { return mRoomType; }

    void absorbRoom(Room *r) override;
    bool removeCoveredTile(Tile* t) override;

    //! Room portal is claimable by enemy seats
    virtual bool isClaimable(Seat* seat) const override;
    virtual void claimForSeat(Seat* seat, Tile* tile, double danceRate) override;

    //! Room portal cannot be destroyed
    virtual bool isAttackable(Tile* tile, Seat* seat) const override
    { return false; }

    //! No seat can sell Room portals
    virtual bool canSeatSellBuilding(Seat* seat) const override
    { return false; }

    //! \brief In addition to the standard upkeep, check to see if a new creature should be spawned.
    void doUpkeep() override;

    //! \brief Creates a new creature whose class is probabalistic and adds it to the game map at the center of the portal.
    void spawnCreature();

    virtual bool displayTileMesh() const override
    { return true; }

    //! \brief Updates the portal position when in editor mode.
    void updateActiveSpots() override;

    virtual void setupRoom(const std::string& name, Seat* seat, const std::vector<Tile*>& tiles) override;

    virtual void restoreInitialEntityState() override;

    static const RoomType mRoomType;

protected:
    virtual void exportToStream(std::ostream& os) const override;
    virtual bool importFromStream(std::istream& is) override;

    void destroyMeshLocal() override;

    void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile) override
    {
        // This Room keeps its building object until it is destroyed (it will be released when
        // the room is destroyed)
    }

private:
    //! \brief Stores the number of turns before spawning the next creature.
    int mSpawnCreatureCountdown;
    BuildingObject* mPortalObject;

    double mClaimedValue;

    uint32_t mNbCreatureMaxIncrease;

    //! \brief Updates the portal mesh position.
    void updatePortalPosition();
};

#endif // ROOMPORTAL_H
