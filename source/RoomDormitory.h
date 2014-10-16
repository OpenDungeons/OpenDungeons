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

#ifndef ROOMDORMITORY_H
#define ROOMDORMITORY_H

#include "Room.h"

//! \brief A class containing info on the bed room objects.
class BedRoomObjectInfo
{
public:
    BedRoomObjectInfo(double x, double y, double rotation, Creature* creature, Tile* tile):
        mX(x),
        mY(y),
        mRotation(rotation),
        mCreature(creature),
        mCentralTile(tile)
    {}

    double getX() const
    {
        return mX;
    }

    double getY() const
    {
        return mY;
    }

    double getRotation() const
    {
        return mRotation;
    }

    Creature* getCreature()
    {
        return mCreature;
    }

    Tile* getCentralTile()
    {
        return mCentralTile;
    }

    const std::vector<Tile*>& getTilesTaken()
    {
        return mTilesTaken;
    }

    void addTileTaken(Tile* tile)
    {
        mTilesTaken.push_back(tile);
    }

private:
    //! \brief Building object position.
    double mX;
    double mY;

    //! \brief Rotation of the model
    double mRotation;

    //! \brief Creature owning the bed
    Creature* mCreature;

    //! \brief The tile owning the bed
    Tile* mCentralTile;

    //! \brief The list of tiles taken by the object
    std::vector<Tile*> mTilesTaken;
};

class RoomDormitory: public Room
{
    friend class ODClient;
public:
    RoomDormitory(GameMap* gameMap);

    virtual RoomType getType() const
    { return RoomType::dormitory; }

    // Functions overriding virtual functions in the Room base class.
    void absorbRoom(Room *r);
    void addCoveredTile(Tile* t, double nHP, bool isRoomAbsorb);
    bool removeCoveredTile(Tile* t, bool isRoomAbsorb);
    void clearCoveredTiles();

    // Functions specific to this class.
    std::vector<Tile*> getOpenTiles();
    bool claimTileForSleeping(Tile *t, Creature *c);
    bool releaseTileForSleeping(Tile *t, Creature *c);
    Tile* getLocationForBed(int xDim, int yDim);

protected:
    void createMeshLocal();

private:
    bool tileCanAcceptBed(Tile *tile, int xDim, int yDim);

    //! \brief Keeps track of the tiles taken by a creature bed
    std::map<Tile*, Creature*> mCreatureSleepingInTile;

    bool installBed(Tile* t, Creature* c, double xDim, double yDim,
        double rotationAngle);
    bool removeBed(Tile *t, Creature *c);

    //! \brief Keeps track of info about the beds in order to be able
    //! to recreate them.
    std::vector<BedRoomObjectInfo> mBedRoomObjectsInfo;
};

#endif // ROOMDORMITORY_H
