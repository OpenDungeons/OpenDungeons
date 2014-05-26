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

#ifndef ROOMQUARTERS_H
#define ROOMQUARTERS_H

#include "Room.h"

class RoomQuarters: public Room
{
public:
    RoomQuarters();

    // Functions overriding virtual functions in the Room base class.
    void absorbRoom(Room *r);
    bool doUpkeep();
    void addCoveredTile(Tile* t, double nHP = defaultRoomTileHP);
    void removeCoveredTile(Tile* t);
    void clearCoveredTiles();

    // Functions specific to this class.
    std::vector<Tile*> getOpenTiles();
    bool claimTileForSleeping(Tile *t, Creature *c);
    bool releaseTileForSleeping(Tile *t, Creature *c);
    Tile* getLocationForBed(int xDim, int yDim);
    void destroyBedMeshes();

private:
    bool tileCanAcceptBed(Tile *tile, int xDim, int yDim);

    std::map<Tile*, Creature*> mCreatureSleepingInTile;
    std::map<Tile*, bool> mBedOrientationForTile;
};

#endif // ROOMQUARTERS_H
