/*!
 * \file   Building.h
 * \date:  22 March 2011
 * \author StefanP.MUC
 * \brief  Provides common methods and members for buildable objects, like rooms and traps
 *
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

#ifndef BUILDING_H_
#define BUILDING_H_

#include "entities/GameEntity.h"

class GameMap;
class RenderedMovableEntity;
class Tile;
class Room;
class Seat;
class Trap;

class TileData
{
public:
    TileData() :
        mHP(0)
    {}

    TileData(const TileData* tileData) :
        mHP(tileData->mHP)
    {}

    virtual ~TileData()
    {}

    virtual TileData* cloneTileData() const
    { return new TileData(this); }

    double mHP;

    //! Seats with vision on the corresponding tile. Note that seats with vision are not copied when cloning a TileData
    std::vector<Seat*> mSeatsVision;
};

/*! \class Building
 *  \brief This class holds elements that are common to Building like Rooms or Traps
 *
 * Functions and properties that are common to every buildable object like rooms and traps
 * should be placed into this class and initialised with a good default value in the default
 * constructor.
 */
class Building : public GameEntity
{
public:
    //! \brief Default constructor with default values. Buildings are used only on server map
    Building(GameMap* gameMap) :
        GameEntity(gameMap, true)
    {}

    virtual ~Building();

    const static double DEFAULT_TILE_HP;

    virtual void doUpkeep() override;

    const Ogre::Vector3& getScale() const;

    //! \brief Updates the active spot lists. Active spots are places where objects can be added
    virtual void updateActiveSpots() = 0;

    //! \brief  Do nothing since Buildings do not have exp.
    void receiveExp(double /*experience*/)
    {}

    //! \brief Checks if the building objects allow the room to be deleted
    bool canBuildingBeRemoved();

    void removeAllBuildingObjects();
    /*! \brief Creates a child RenderedMovableEntity mesh using the given mesh name and placing on the target tile,
     *  if the tile is nullptr the object appears in the building's center, the rotation angle is given in degrees.
     */
    RenderedMovableEntity* loadBuildingObject(GameMap* gameMap, const std::string& meshName,
        Tile* targetTile, double rotationAngle, bool hideCoveredTile, float opacity = 1.0f,
        const std::string& initialAnimationState = "", bool initialAnimationLoop = true);
    RenderedMovableEntity* loadBuildingObject(GameMap* gameMap, const std::string& meshName,
        Tile* targetTile, double x, double y, double rotationAngle, bool hideCoveredTile, float opacity = 1.0f,
        const std::string& initialAnimationState = "", bool initialAnimationLoop = true);
    Tile* getCentralTile();

    virtual bool isClaimable(Seat* seat) const
    { return false; }

    virtual void claimForSeat(Seat* seat, Tile* tile, double danceRate)
    {}

    virtual bool canSeatSellBuilding(Seat* seat) const;

    virtual bool isAttackable(Tile* tile, Seat* seat) const;
    virtual bool removeCoveredTile(Tile* t);
    std::vector<Tile*> getCoveredTiles();
    Tile* getCoveredTile(int index);
    uint32_t numCoveredTiles()
    { return mCoveredTiles.size(); }

    virtual void clearCoveredTiles();
    double getHP(Tile *tile) const;
    double takeDamage(GameEntity* attacker, double physicalDamage, double magicalDamage, Tile *tileTakingDamage);
    std::string getNameTile(Tile* tile);

    //! \brief Tells whether the building tile should be displayed.
    virtual bool shouldDisplayBuildingTile() const
    { return true; }

    //! \brief Tells whether the ground tile below the building tile should be displayed.
    virtual bool shouldDisplayGroundTile() const
    { return false; }

    //! \brief Tells whether the building wants the given entity to be brought
    virtual bool hasCarryEntitySpot(GameEntity* carriedEntity)
    { return false; }

    //! \brief Tells where the building wants the given entity to be brought
    //! returns the Tile carriedEntity should be brought to or nullptr if
    //! the carriedEntity is not wanted anymore (if no free spot for example).
    //! If askSpotForCarriedEntity returns a valid tile, a spot may be booked and in
    //! any case, notifyCarryingStateChanged should be called to release it
    virtual Tile* askSpotForCarriedEntity(GameEntity* carriedEntity)
    { return nullptr; }

    //! \brief Tells whether the carrying state changed. One should check the carrier
    //! tile position to check if it is in the requested Tile. If yes, the carriedEntity
    //! is at the wanted place. If not, it means that the carrier stopped carrying (for
    //! example, if it was killed or picked up during process)
    virtual void notifyCarryingStateChanged(Creature* carrier, GameEntity* carriedEntity)
    {}

    //! Tells if the covering tile should be set to dirty when the building is added on the tile
    virtual bool shouldSetCoveringTileDirty(Seat* seat, Tile* tile)
    { return true; }

    inline const std::vector<Tile*>& getCoveredTilesDestroyed() const
    { return mCoveredTilesDestroyed; }

    virtual bool isTileVisibleForSeat(Tile* tile, Seat* seat) const
    { return true; }

    virtual void notifySeatVision(Tile* tile, Seat* seat);

    virtual bool canCreatureGoThroughTile(const Creature* creature, Tile* tile) const
    { return true; }

    virtual bool permitsVision(Tile* tile)
    { return true; }

protected:
    virtual void exportToStream(std::ostream& os) const override;
    virtual void importFromStream(std::istream& is) override;
    //! Allows to export/import specific data for child classes. Note that every tile
    //! should be exported on 1 line (thus, no line ending should be added here). Moreover
    //! the building will only export the tile coords. Exporting other relevant data is
    //! up to the subclass
    virtual void exportTileDataToStream(std::ostream& os, Tile* tile, TileData* tileData) const
    {}
    //! importTileDataFromStream should add the tile to covered or destroyed tiles vector and,
    //! if added to covered tile vectors, set covering room
    virtual void importTileDataFromStream(std::istream& is, Tile* tile, TileData* tileData)
    {}

    //! This will be called when tiles will be added to the building. By overriding it,
    //! child classes can expand TileData and add the data they need
    virtual TileData* createTileData(Tile* tile);

    void addBuildingObject(Tile* targetTile, RenderedMovableEntity* obj);
    void removeBuildingObject(Tile* tile);
    void removeBuildingObject(RenderedMovableEntity* obj);
    RenderedMovableEntity* getBuildingObjectFromTile(Tile* tile);
    //! Buildings are handled by the tile, they don't fire add/remove events
    void fireAddEntity(Seat* seat, bool async)
    {}
    void fireRemoveEntity(Seat* seat)
    {}

    std::map<Tile*, RenderedMovableEntity*> mBuildingObjects;
    std::vector<Tile*> mCoveredTiles;
    std::vector<Tile*> mCoveredTilesDestroyed;
    std::map<Tile*, TileData*> mTileData;
};

#endif // BUILDING_H_
