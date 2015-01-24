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

#ifndef ROOMDUNGEONTEMPLE_H
#define ROOMDUNGEONTEMPLE_H

#include "rooms/Room.h"

class RoomDungeonTemple: public Room
{
public:
    RoomDungeonTemple(GameMap* gameMap);

    virtual RoomType getType() const
    { return RoomType::dungeonTemple; }

    //! \brief Updates the temple position when in editor mode.
    void updateActiveSpots();

    /*! \brief Counts down a timer until it reaches 0,
     * then it spawns a kobold of the color of this dungeon temple
     * at the center of the dungeon temple, and resets the timer.
     */
    void produceKobold();

protected:
    virtual void createMeshLocal();
    virtual void destroyMeshLocal();

    void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
    {
        // This Room keeps its building object until it is destroyed (they will be released when
        // the room is destroyed)
    }

private:
    //! \brief The number of turns to wait before producing a worker
    int mWaitTurns;

    //! \brief The reference of the temple object
    RenderedMovableEntity* mTempleObject;

    //! \brief Updates the temple mesh position.
    void updateTemplePosition();
};

#endif // ROOMDUNGEONTEMPLE_H
