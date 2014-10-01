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

#include "RoomDormitory.h"

#include "ODServer.h"
#include "ServerNotification.h"
#include "Tile.h"
#include "GameMap.h"
#include "RoomObject.h"
#include "Creature.h"
#include "LogManager.h"

RoomDormitory::RoomDormitory(GameMap* gameMap) :
    Room(gameMap)
{
    mType = dormitory;
}

void RoomDormitory::absorbRoom(Room *r)
{
    RoomDormitory* oldRoom = static_cast<RoomDormitory*>(r);
    if (oldRoom == NULL)
        return;

    // We transfert the room objects
    mCreatureSleepingInTile.insert(oldRoom->mCreatureSleepingInTile.begin(), oldRoom->mCreatureSleepingInTile.end());
    oldRoom->mCreatureSleepingInTile.clear();

    mBedRoomObjectsInfo.insert(mBedRoomObjectsInfo.end(),
        oldRoom->mBedRoomObjectsInfo.begin(), oldRoom->mBedRoomObjectsInfo.end());
    oldRoom->mBedRoomObjectsInfo.clear();

    // This function will copy the room objects (beds) into the new room
    // and remove the old room covered tiles.
    Room::absorbRoom(r);
}

void RoomDormitory::createMeshLocal()
{
    Room::createMeshLocal();

    // The client game map should not load room objects. They will be created
    // by the messages sent by the server because some of them are randomly
    // created
    if(!getGameMap()->isServerGameMap())
        return;

    // Recreate the meshes with the new size.
    if (mCoveredTiles.empty())
        return;

    // Recreate every room objects
    for (unsigned int i = 0; i < mBedRoomObjectsInfo.size(); ++i)
    {
        Creature* creature = mBedRoomObjectsInfo[i].getCreature();
        if (creature == NULL)
            continue;

        const CreatureDefinition* def = creature->getDefinition();
        if (def == NULL)
            continue;

        double x = mBedRoomObjectsInfo[i].getX();
        double y = mBedRoomObjectsInfo[i].getY();
        double rotationAngle = mBedRoomObjectsInfo[i].getRotation();
        Tile* tile = mBedRoomObjectsInfo[i].getCentralTile();
        // Tell the creature about its setHomeTile
        creature->setHomeTile(tile);

        // Reload the creature taking tiles data.
        const std::vector<Tile*>& tilesTaken = mBedRoomObjectsInfo[i].getTilesTaken();
        for (unsigned int j = 0; j < tilesTaken.size(); ++j)
            mCreatureSleepingInTile[tilesTaken[j]] = creature;

        // And recreate the bed room object
        RoomObject* ro = loadRoomObject(getGameMap(), def->getBedMeshName(), tile, x, y, rotationAngle);
        addRoomObject(tile, ro);
        ro->createMesh();
    }

    // Add NULL creature in the mCreatureSleepingInTile map where there a nobody,
    // so that getOpenTiles() can later find empty slots for new beds.
    for (unsigned int i = 0; i < mCoveredTiles.size(); ++i)
    {
        Tile* tile = mCoveredTiles[i];
        if (mCreatureSleepingInTile.find(tile) != mCreatureSleepingInTile.end())
            continue;

        mCreatureSleepingInTile[tile] = NULL;
    }
}

void RoomDormitory::addCoveredTile(Tile* t, double nHP, bool isRoomAbsorb)
{
    Room::addCoveredTile(t, nHP, isRoomAbsorb);

    // Only initialize the tile to NULL if it is a tile being added to a new room.  If it is being absorbed
    // from another room the map value will already have been set and we don't want to override it.
    if (!isRoomAbsorb)
        mCreatureSleepingInTile[t] = NULL;
}

bool RoomDormitory::removeCoveredTile(Tile* t, bool isRoomAbsorb)
{
    if (t == NULL)
        return false;

    if(mCreatureSleepingInTile.count(t) > 0)
    {
        Creature* c = mCreatureSleepingInTile[t];
        OD_ASSERT_TRUE(c != NULL);
        if (c != NULL)
        {
            // Inform the creature that it no longer has a place to sleep
            // and remove the bed tile.
            releaseTileForSleeping(t, c);
        }
    }

    if(Room::removeCoveredTile(t, isRoomAbsorb))
    {
        mCreatureSleepingInTile.erase(t);
        return true;
    }

    return false;
}

void RoomDormitory::clearCoveredTiles()
{
    Room::clearCoveredTiles();
    mCreatureSleepingInTile.clear();
}

std::vector<Tile*> RoomDormitory::getOpenTiles()
{
    std::vector<Tile*> returnVector;

    for (std::map<Tile*, Creature*>::iterator itr = mCreatureSleepingInTile.begin();
         itr != mCreatureSleepingInTile.end(); ++itr)
    {
        if (itr->second == NULL)
            returnVector.push_back(itr->first);
    }

    return returnVector;
}

bool RoomDormitory::claimTileForSleeping(Tile* t, Creature* c)
{
    if (t == NULL || c == NULL)
        return false;

    // Check to see if there is already a creature which has claimed this tile for sleeping.
    if (mCreatureSleepingInTile[t] != NULL)
        return false;

    const CreatureDefinition* def = c->getDefinition();
    if (!def)
        return false;

    double xDim, yDim, rotationAngle = 0.0;
    bool spaceIsBigEnough = false;

    // Check to see whether the bed should be situated x-by-y or y-by-x tiles.
    if (tileCanAcceptBed(t, def->getBedDim1(), def->getBedDim2()))
    {
        spaceIsBigEnough = true;
        xDim = def->getBedDim1();
        yDim = def->getBedDim2();
        rotationAngle = 0.0;
    }

    if (!spaceIsBigEnough && tileCanAcceptBed(t, def->getBedDim2(), def->getBedDim1()))
    {
        spaceIsBigEnough = true;
        xDim = def->getBedDim2();
        yDim = def->getBedDim1();
        rotationAngle = 90.0;
    }

    if (!spaceIsBigEnough)
        return false;

    return installBed(t, c, xDim, yDim, rotationAngle);
}

bool RoomDormitory::releaseTileForSleeping(Tile* t, Creature* c)
{
    // If the creature is not in the gamemap anymore, no need to notify that its bed
    // is to be removed as the clients will handle that by themselves
    if(getGameMap()->getCreature(c->getName()) == NULL)
        return true;

    return removeBed(t, c);
}

bool RoomDormitory::installBed(Tile* t, Creature* c, double xDim, double yDim,
    double rotationAngle)
{
    const CreatureDefinition* def = c->getDefinition();
    if(def == NULL)
        return false;

    BedRoomObjectInfo bedInfo(t->x + xDim / 2.0 - 0.5,
                              t->y + yDim / 2.0 - 0.5,
                              rotationAngle, c, t);

    // Mark all of the affected tiles as having this creature sleeping in them.
    for (int i = 0; i < xDim; ++i)
    {
        for (int j = 0; j < yDim; ++j)
        {
            Tile *tempTile = getGameMap()->getTile(t->x + i, t->y + j);
            mCreatureSleepingInTile[tempTile] = c;
            bedInfo.addTileTaken(tempTile);
        }
    }

    // Add the model
    RoomObject* ro = loadRoomObject(getGameMap(), def->getBedMeshName(), t, bedInfo.getX(), bedInfo.getY(), rotationAngle);
    addRoomObject(t, ro);
    ro->createMesh();
    // Save the info for later...
    mBedRoomObjectsInfo.push_back(bedInfo);
    return true;
}

bool RoomDormitory::removeBed(Tile* t, Creature* c)
{
    if (c == NULL)
        return false;

    if (mCreatureSleepingInTile.find(t) == mCreatureSleepingInTile.end())
        return false;

    // Loop over all the tiles in this room and if they are slept on by creature c then set them back to NULL.
    for (std::map<Tile*, Creature*>::iterator itr = mCreatureSleepingInTile.begin();
         itr != mCreatureSleepingInTile.end(); ++itr)
    {
        if (itr->second == c)
        {
            itr->second = NULL;
        }
    }

    c->setHomeTile(NULL);

    // Make the room object delete itself and remove it from the map
    RoomObject* roomObject = getRoomObjectFromTile(t);
    removeRoomObject(roomObject);

    // Remove the bedinfo as well
    std::vector<BedRoomObjectInfo>::iterator it2 = mBedRoomObjectsInfo.begin();
    while (it2 != mBedRoomObjectsInfo.end())
    {
        if ((*it2).getCentralTile() == t)
        {
            it2 = mBedRoomObjectsInfo.erase(it2);
        }
        else
        {
            ++it2;
        }
    }

    return true;
}

Tile* RoomDormitory::getLocationForBed(int xDim, int yDim)
{
    // Force the dimensions to be positive.
    if (xDim < 0)
        xDim *= -1;
    if (yDim < 0)
        yDim *= -1;

    // Check to see if there is even enough space available for the bed.
    std::vector<Tile*> tempVector = getOpenTiles();
    unsigned int area = xDim * yDim;
    if (tempVector.size() < area)
        return NULL;

    // Randomly shuffle the open tiles in tempVector so that the dormitory are filled up in a random order.
    std::random_shuffle(tempVector.begin(), tempVector.end());

    // Loop over each of the open tiles in tempVector and for each one, check to see if it
    for (unsigned int i = 0; i < tempVector.size(); ++i)
    {
        if (tileCanAcceptBed(tempVector[i], xDim, yDim))
            return tempVector[i];
    }

    // We got to the end of the open tile list without finding an open tile for the bed so return NULL to indicate failure.
    return NULL;
}

bool RoomDormitory::tileCanAcceptBed(Tile *tile, int xDim, int yDim)
{
    //TODO: This function could be made more efficient by making it take the list of open tiles as an argument so if it is called repeatedly the tempTiles vecotor below only has to be computed once in the calling function rather than N times in this function.

    // Force the dimensions to be positive.
    if (xDim < 0)
        xDim *= -1;
    if (yDim < 0)
        yDim *= -1;

    // If either of the dimensions is 0 just return true, since the bed takes no space.  This should never really happen anyway.
    if (xDim == 0 || yDim == 0)
        return true;

    // If the tile is invalid or not part of this room then the bed cannot be placed in this room.
    if (tile == NULL || tile->getCoveringRoom() != this)
        return false;

    // Create a 2 dimensional array of booleans initially all set to false.
    std::vector<std::vector<bool> > tileOpen(xDim);
    for (int i = 0; i < xDim; ++i)
    {
        tileOpen[i].resize(yDim, false);
    }

    // Now loop over the list of all the open tiles in this dormitory.  For each tile, if it falls within
    // the xDim by yDim area from the starting tile we set the corresponding tileOpen entry to true.
    std::vector<Tile*> tempTiles = getOpenTiles();
    for (unsigned int i = 0; i < tempTiles.size(); ++i)
    {
        int xDist = tempTiles[i]->x - tile->x;
        int yDist = tempTiles[i]->y - tile->y;
        if (xDist >= 0 && xDist < xDim && yDist >= 0 && yDist < yDim)
            tileOpen[xDist][yDist] = true;
    }

    // Loop over the tileOpen array and check to see if every value has been set to true, if it has then
    // we can place the a bed of the specified dimensions with its corner at the specified starting tile.
    bool returnValue = true;
    for (int i = 0; i < xDim; ++i)
    {
        for (int j = 0; j < yDim; ++j)
        {
            returnValue = returnValue && tileOpen[i][j];
        }
    }

    return returnValue;
}
