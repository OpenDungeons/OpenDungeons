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
    BedRoomObjectInfo(double x, double y, double rotation, Creature* creature, Tile* tile):
        mX(x),
        mY(y),
        mRotation(rotation),
        mCreature(creature),
        mOwningTile(tile)
    {}

    inline double getX() const
    { return mX; }

    inline double getY() const
    { return mY; }

    inline double getRotation() const
    { return mRotation; }

    inline Creature* getCreature() const
    { return mCreature; }

    inline Tile* getOwningTile() const
    { return mOwningTile; }

    inline const std::vector<Tile*>& getTilesTaken() const
    { return mTilesTaken; }

    inline void addTileTaken(Tile* tile)
    { mTilesTaken.push_back(tile); }

private:
    //! \brief Building object position.
    double mX;
    double mY;

    //! \brief Rotation of the model
    double mRotation;

    //! \brief Creature owning the bed
    Creature* mCreature;

    //! \brief The tile owning the bed
    Tile* mOwningTile;

    //! \brief The list of tiles taken by the object
    std::vector<Tile*> mTilesTaken;
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

    virtual RoomType getType() const override
    { return RoomType::dormitory; }

    // Functions overriding virtual functions in the Room base class.
    void absorbRoom(Room *r) override;
    bool removeCoveredTile(Tile* t) override;

    virtual void restoreInitialEntityState() override;

    // Functions specific to this class.
    std::vector<Tile*> getOpenTiles();
    bool claimTileForSleeping(Tile *t, Creature *c);
    bool releaseTileForSleeping(Tile *t, Creature *c);
    Tile* getLocationForBed(int xDim, int yDim);

    bool hasCarryEntitySpot(GameEntity* carriedEntity) override;
    Tile* askSpotForCarriedEntity(GameEntity* carriedEntity) override;
    void notifyCarryingStateChanged(Creature* carrier, GameEntity* carriedEntity) override;

    static void checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);
    static bool buildRoom(GameMap* gameMap, Player* player, ODPacket& packet);
    static bool buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles);
    static void checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);
    static bool buildRoomEditor(GameMap* gameMap, ODPacket& packet);
    static Room* getRoomFromStream(GameMap* gameMap, std::istream& is);

protected:
    virtual void exportToStream(std::ostream& os) const override;
    virtual void importFromStream(std::istream& is) override;

    RoomDormitoryTileData* createTileData(Tile* tile);
    // Because dormitory do not use active spots, we don't want the default
    // behaviour (removing the active spot tile) as it could result in removing an
    // unwanted bed
    void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile) override
    {}

private:
    bool tileCanAcceptBed(Tile *tile, int xDim, int yDim);
    void createBed(Tile* t, double rotationAngle, Creature* c);

    //! \brief Keeps track of info about the beds in order to be able
    //! to recreate them.
    std::vector<BedRoomObjectInfo> mBedRoomObjectsInfo;

    //! Used at map load to store information about beds already in this room
    std::vector<BedCreatureLoad> mBedCreatureLoad;
};

#endif // ROOMDORMITORY_H
