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

#include "RoomLibrary.h"

#include "Tile.h"
#include "GameMap.h"
#include "RoomObject.h"
#include "Random.h"

RoomLibrary::RoomLibrary(GameMap* gameMap) :
    Room(gameMap)
{
    mType = library;
}

RoomObject* RoomLibrary::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    switch(place)
    {
        case ActiveSpotPlace::activeSpotCenter:
        {
        if (Random::Int(0, 100) > 50)
            return loadRoomObject(getGameMap(), "Podium", tile);
        else
            return loadRoomObject(getGameMap(), "Bookcase", tile);
        }
        case ActiveSpotPlace::activeSpotLeft:
        {
            return loadRoomObject(getGameMap(), "Bookshelf", tile, 90.0);
        }
        case ActiveSpotPlace::activeSpotRight:
        {
            return loadRoomObject(getGameMap(), "Bookshelf", tile, 270.0);
        }
        case ActiveSpotPlace::activeSpotTop:
        {
            return loadRoomObject(getGameMap(), "Bookshelf", tile, 0.0);
        }
        case ActiveSpotPlace::activeSpotBottom:
        {
            return loadRoomObject(getGameMap(), "Bookshelf", tile, 180.0);
        }
    }
    return NULL;
}

