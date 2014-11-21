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

#include "rooms/RoomCrypt.h"

#include "entities/Tile.h"
#include "gamemap/GameMap.h"
#include "utils/Random.h"

RoomCrypt::RoomCrypt(GameMap* gameMap) :
    Room(gameMap)
{
    setMeshName("Crypt");
}

RenderedMovableEntity* RoomCrypt::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    switch(place)
    {
        case ActiveSpotPlace::activeSpotCenter:
        {
            int rnd = Random::Int(0, 100);
            if (rnd < 33)
                return loadBuildingObject(getGameMap(), "KnightCoffin", tile, 0.0, false);
            else if (rnd < 66)
                return loadBuildingObject(getGameMap(), "CelticCross", tile, 0.0, false);
            else
                return loadBuildingObject(getGameMap(), "StoneCoffin", tile, 0.0, false);
        }
        case ActiveSpotPlace::activeSpotLeft:
        {
            return loadBuildingObject(getGameMap(), "KnightStatue", tile, 90.0, false);
        }
        case ActiveSpotPlace::activeSpotRight:
        {
            return loadBuildingObject(getGameMap(), "KnightStatue", tile, 270.0, false);
        }
        case ActiveSpotPlace::activeSpotTop:
        {
            return loadBuildingObject(getGameMap(), "KnightStatue2", tile, 0.0, false);
        }
        case ActiveSpotPlace::activeSpotBottom:
        {
            return loadBuildingObject(getGameMap(), "KnightStatue2", tile, 180.0, false);
        }
    }
    return NULL;
}

