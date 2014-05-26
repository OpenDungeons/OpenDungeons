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

#ifndef ROOMDUNGEONTEMPLE_H
#define ROOMDUNGEONTEMPLE_H

#include "Room.h"

class RoomDungeonTemple: public Room
{
public:
    RoomDungeonTemple();

    void createMesh();
    void destroyMesh();

    /*! \brief Counts down a timer until it reaches 0,
        *  then it spawns a kobold of the color of this dungeon temple
        *  at the center of the dungeon temple, and resets the timer.
        */
    void produceKobold();

private:
    int mWaitTurns;
};

#endif // ROOMDUNGEONTEMPLE_H
