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

#ifndef ROOMTREASURY_H
#define ROOMTREASURY_H

#include "entities/TreasuryObject.h"
#include "rooms/Room.h"

class RoomTreasury: public Room
{
    friend class ODClient;
public:
    RoomTreasury(GameMap* gameMap);

    virtual RoomType getType() const
    { return RoomType::treasury; }

    // Functions overriding virtual functions in the Room base class.
    void absorbRoom(Room *r);
    void addCoveredTile(Tile* t, double nHP);
    bool removeCoveredTile(Tile* t);

    // Functions specific to this class.
    int getTotalGold();
    int emptyStorageSpace();
    int depositGold(int gold, Tile *tile);
    int withdrawGold(int gold);
    virtual void doUpkeep();

    bool hasCarryEntitySpot(MovableGameEntity* carriedEntity);
    Tile* askSpotForCarriedEntity(MovableGameEntity* carriedEntity);
    void notifyCarryingStateChanged(Creature* carrier, MovableGameEntity* carriedEntity);

protected:
    // Because treasury do not use active spots, we don't want the default
    // behaviour (removing the active spot tile) as it could result in removing an
    // unwanted treasury
    void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
    {}

private:
    void updateMeshesForTile(Tile *t);

    std::map<Tile*, int> mGoldInTile;
    std::map<Tile*, TreasuryObject::TreasuryTileFullness> mFullnessOfTile;
    bool mGoldChanged;
};

#endif // ROOMTREASURY_H
