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

#include "rooms/RoomLibrary.h"

#include "entities/Tile.h"
#include "gamemap/GameMap.h"
#include "utils/Random.h"

RoomLibrary::RoomLibrary(GameMap* gameMap) :
    Room(gameMap)
{
    setMeshName("Library");
}

RenderedMovableEntity* RoomLibrary::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    switch(place)
    {
        case ActiveSpotPlace::activeSpotCenter:
        {
        if (Random::Int(0, 100) > 50)
            return loadBuildingObject(getGameMap(), "Podium", tile, 0.0, false);
        else
            return loadBuildingObject(getGameMap(), "Bookcase", tile, 0.0, false);
        }
        case ActiveSpotPlace::activeSpotLeft:
        {
            return loadBuildingObject(getGameMap(), "Bookshelf", tile, 90.0, false);
        }
        case ActiveSpotPlace::activeSpotRight:
        {
            return loadBuildingObject(getGameMap(), "Bookshelf", tile, 270.0, false);
        }
        case ActiveSpotPlace::activeSpotTop:
        {
            return loadBuildingObject(getGameMap(), "Bookshelf", tile, 0.0, false);
        }
        case ActiveSpotPlace::activeSpotBottom:
        {
            return loadBuildingObject(getGameMap(), "Bookshelf", tile, 180.0, false);
        }
    }
    return nullptr;
}

