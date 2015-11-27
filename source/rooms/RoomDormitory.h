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

#ifndef ROOMDORMITORY_H
#define ROOMDORMITORY_H

#include "rooms/Room.h"
#include "rooms/RoomType.h"

#include <OgreVector3.h>

class RoomDormitoryTileData : public TileData
{
public:
    RoomDormitoryTileData() :
        TileData(),
        mCreature(nullptr)
    {}

    RoomDormitoryTileData(const RoomDormitoryTileData* roomDormitoryTileData) :
        TileData(roomDormitoryTileData),
        mCreature(roomDormitoryTileData->mCreature)
    {}

    virtual ~RoomDormitoryTileData()
    {}

    virtual RoomDormitoryTileData* cloneTileData() const override
    { return new RoomDormitoryTileData(this); }

    Creature* mCreature;
};

//! \brief A class containing info on the bed room objects.
class BedRoomObjectInfo
{
public:
    BedRoomObjectInfo(int x, int y, double rotation, Creature* creature, Tile* tile, const Ogre::Vector3& sleepDirection):
        mX(x),
        mY(y),
        mRotation(rotation),
        mCreature(creature),
        mOwningTile(tile),
        mSleepDirection(sleepDirection)
    {}

    inline int getX() const
    { return mX; }

    inline int getY() const
    { return mY; }

    inline double getRotation() const
    { return mRotation; }

    inline Creature* getCreature() const
    { return mCreature; }

    inline Tile* getOwningTile() const
    { return mOwningTile; }


    inline const std::vector<Tile*>& getTilesTaken() const
    { return mTilesTaken; }

    inline const Ogre::Vector3& getSleepDirection() const
    { return mSleepDirection; }

    inline void addTileTaken(Tile* tile)
    { mTilesTaken.push_back(tile); }

private:
    //! \brief Position of bottom left of the bed
    int mX;
    int mY;

    //! \brief Rotation of the model
    double mRotation;

    //! \brief Creature owning the bed
    Creature* mCreature;

    //! \brief The tile owning the bed
    Tile* mOwningTile;

    //! \brief The list of tiles taken by the object
    std::vector<Tile*> mTilesTaken;

    Ogre::Vector3 mSleepDirection;
};

/*! Class used at room loading to save the data needed to recreate the beds after map loading when
 * restoreInitialEntityState is called
 */
class BedCreatureLoad
{
public:
    BedCreatureLoad(const std::string& creatureName, int tileX, int tileY, double rotationAngle) :
        mCreatureName(creatureName),
        mTileX(tileX),
        mTileY(tileY),
        mRotationAngle(rotationAngle)
    {}

    inline int getTileX() const
    { return mTileX; }

    inline int getTileY() const
    { return mTileY; }

    inline const std::string& getCreatureName() const
    { return mCreatureName; }

    inline double getRotationAngle() const
    { return mRotationAngle; }

private:
    std::string mCreatureName;
    int mTileX;
    int mTileY;
    double mRotationAngle;
};

class RoomDormitory: public Room
{
public:
    RoomDormitory(GameMap* gameMap);

    RoomType getType() const override
    { return mRoomType; }

    void absorbRoom(Room *r) override;
    bool removeCoveredTile(Tile* t) override;

    void restoreInitialEntityState() override;

    // Functions specific to this class.
    std::vector<Tile*> getOpenTiles();
    Tile* claimTileForSleeping(Tile *t, Creature *c);
    bool releaseTileForSleeping(Tile *t, Creature *c);
    Tile* getLocationForBed(Creature* creature);
    const Ogre::Vector3& getSleepDirection(Creature* creature) const;

    bool hasCarryEntitySpot(GameEntity* carriedEntity) override;
    Tile* askSpotForCarriedEntity(GameEntity* carriedEntity) override;
    void notifyCarryingStateChanged(Creature* carrier, GameEntity* carriedEntity) override;
    bool isRestRoom(Creature& creature) override
    { return true; }

    static const RoomType mRoomType;

protected:
    void exportToStream(std::ostream& os) const override;
    bool importFromStream(std::istream& is) override;

    RoomDormitoryTileData* createTileData(Tile* tile);
    // Because dormitory do not use active spots, we don't want the default
    // behaviour (removing the active spot tile) as it could result in removing an
    // unwanted bed
    void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile) override
    {}

private:
    bool tileCanAcceptBed(Tile *tile, int xDim, int yDim, const std::vector<Tile*>& openTiles);
    void createBed(Tile* sleepTile, int x, int y, int width, int height,
        double rotationAngle, Creature* c, const Ogre::Vector3& sleepDirection);

    //! \brief Keeps track of info about the beds in order to be able
    //! to recreate them.
    std::vector<BedRoomObjectInfo> mBedRoomObjectsInfo;

    //! Used at map load to store information about beds already in this room
    std::vector<BedCreatureLoad> mBedCreatureLoad;
};

#endif // ROOMDORMITORY_H
