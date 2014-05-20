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


    for(unsigned int i = 0, size = mCentralActiveSpotTiles.size(); i < size; ++i)
    {
        loadRoomObject("TrainingDummy", mCentralActiveSpotTiles[i]);
    }

    createRoomObjectMeshes();
}

int RoomDojo::numOpenCreatureSlots()
{
    // TODO: Later, add other active spots to the count.
    return mCentralActiveSpotTiles.size() - numCreaturesUsingRoom();
}
