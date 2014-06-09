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

#ifndef ROOMPORTAL_H
#define ROOMPORTAL_H

#include "Room.h"
#include "CreatureDefinition.h"

#include <vector>
#include <map> //For pair

class RoomPortal: public Room
{
public:
    RoomPortal();

    // Functions overriding virtual functions in the Room base class.
    void createMesh();
    void addCoveredTile(Tile* t, double nHP = defaultRoomTileHP);
    void removeCoveredTile(Tile* t);

    //! \brief In addition to the standard upkeep, check to see if a new creature should be spawned.
    bool doUpkeep();

    //! \brief Creates a new creature whose class is probabalistic and adds it to the game map at the center of the portal.
    void spawnCreature();

private:
    //! \brief Finds the X,Y coordinates of the center of the tiles that make up the portal.
    void recomputeCenterPosition();

    int mSpawnCreatureCountdown;

    double mXCenter;
    double mYCenter;

    RoomObject* mPortalObject;
};

#endif // ROOMPORTAL_H
