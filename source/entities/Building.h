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
#include "game/Seat.h"

class GameMap;
class RenderedMovableEntity;
class Tile;
class Room;
class Trap;

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
    //! \brief Default constructor with default values
    Building(GameMap* gameMap) :
        GameEntity(gameMap)
    {}

    const static double DEFAULT_TILE_HP;

    virtual ~Building() {}

    const Ogre::Vector3& getScale() const;

    //! \brief Updates the active spot lists. Active spots are places where objects can be added
    virtual void updateActiveSpots() = 0;

    //! \brief  Do nothing since Buildings do not have exp.
    void receiveExp(double /*experience*/)
    {}

    void removeAllBuildingObjects();
    /*! \brief Creates a child RenderedMovableEntity mesh using the given mesh name and placing on the target tile,
     *  if the tile is nullptr the object appears in the building's center, the rotation angle is given in degrees.
     */
    RenderedMovableEntity* loadBuildingObject(GameMap* gameMap, const std::string& meshName,
        Tile* targetTile, double rotationAngle, bool hideCoveredTile, float opacity = 1.0f);
    RenderedMovableEntity* loadBuildingObject(GameMap* gameMap, const std::string& meshName,
        Tile* targetTile, double x, double y, double rotationAngle, bool hideCoveredTile, float opacity = 1.0f);
    Tile* getCentralTile();

    virtual bool isAttackable(Tile* tile, Seat* seat) const;
    virtual void addCoveredTile(Tile* t, double nHP);
    virtual bool removeCoveredTile(Tile* t);
    std::vector<Tile*> getCoveredTiles();
    Tile* getCoveredTile(int index);
    uint32_t numCoveredTiles();
    virtual void clearCoveredTiles();
    double getHP(Tile *tile) const;
    double takeDamage(GameEntity* attacker, double physicalDamage, double magicalDamage, Tile *tileTakingDamage);
    std::string getNameTile(Tile* tile);

    //! \brief Tells whether the building tile should be displayed.
    virtual bool shouldDisplayBuildingTile()
    {
        return true;
    }

    //! \brief Tells whether the ground tile below the building tile should be displayed.
    virtual bool shouldDisplayGroundTile()
    {
        return false;
    }

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

    //! Notify the seats that have vision on the given tile
    virtual void notifySeatsVisionOnTile(const std::vector<Seat*>& seats, Tile* tile);

    inline const std::vector<Tile*>& getCoveredTilesDestroyed() const
    { return mCoveredTilesDestroyed; }

protected:
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
    std::map<Tile*, double> mTileHP;
};

#endif // BUILDING_H_
