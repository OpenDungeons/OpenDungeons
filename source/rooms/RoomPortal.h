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
#include "entities/CreatureDefinition.h"

#include <vector>
#include <map> //For pair

class RoomPortal: public Room
{
public:
    RoomPortal(GameMap* gameMap);

    virtual RoomType getType() const
    { return RoomType::portal; }

    //! \brief In addition to the standard upkeep, check to see if a new creature should be spawned.
    void doUpkeep();

    //! \brief Creates a new creature whose class is probabalistic and adds it to the game map at the center of the portal.
    void spawnCreature();

    //! \brief Portals only display claimed tiles on their ground.
    virtual bool shouldDisplayBuildingTile()
    {
        return false;
    }

    //! \brief Updates the portal position when in editor mode.
    void updateActiveSpots();

protected:
    void destroyMeshLocal();

    void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
    {
        // This Room keeps its building object until it is destroyed (it will be released when
        // the room is destroyed)
    }

private:
    //! \brief Stores the number of turns before spawning the next creature.
    int mSpawnCreatureCountdown;

    RenderedMovableEntity* mPortalObject;

    //! \brief Updates the portal mesh position.
    void updatePortalPosition();
};

#endif // ROOMPORTAL_H
