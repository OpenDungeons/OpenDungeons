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

#include "RoomDojo.h"

#include "RoomObject.h"
#include "Tile.h"
#include "GameMap.h"

RoomDojo::RoomDojo()
{
    mType = dojo;
}

bool sortCoveredTiles(Tile* i, Tile* j)
{
    if (i->getY() < j->getY())
        return true;
    else if (i->getY() > j->getY())
        return false;

    // i:y == j:y
   if (i->getX() < j->getX())
        return true;

    return false;
}

void RoomDojo::createMesh()
{
    Room::createMesh();

    // Clean everything
    std::map<Tile*, RoomObject*>::iterator itr = mRoomObjects.begin();
    while (itr != mRoomObjects.end())
    {
        itr->second->deleteYourself();
        ++itr;
    }
    mRoomObjects.clear();

    // And recreate the meshes with the new size.
    if (mCoveredTiles.empty())
        return;

    // Sort the tiles by coordinates
    std::sort(mCoveredTiles.begin(), mCoveredTiles.end(), sortCoveredTiles);

    bool place_it = false;
    for(unsigned int i = 0, size = mCoveredTiles.size(); i < size; ++i)
    {
        if (place_it)
            loadRoomObject("TrainingDummy", mCoveredTiles[i]);
        place_it = !place_it;
    }

    createRoomObjectMeshes();
}

int RoomDojo::numOpenCreatureSlots()
{
    return 3 - numCreaturesUsingRoom();
}
