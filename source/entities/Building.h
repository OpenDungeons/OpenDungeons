/*!
 * \file   Building.h
 * \date:  22 March 2011
 * \author StefanP.MUC
 * \brief  Provides common methods and members for buildable objects, like rooms and traps
 *
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

#ifndef BUILDING_H_
#define BUILDING_H_

#include "entities/GameEntity.h"
#include "game/Seat.h"

class GameMap;
class RenderedMovableEntity;
class Tile;

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

    //! \brief Updates the active spot lists. Active spots are places where objects can be added
    virtual void updateActiveSpots() = 0;

    //! \brief  Do nothing since Buildings do not have exp.
    void receiveExp(double /*experience*/)
    {}

    void addBuildingObject(Tile* targetTile, RenderedMovableEntity* obj);
    void removeBuildingObject(Tile* tile);
    void removeBuildingObject(RenderedMovableEntity* obj);
    void removeAllBuildingObjects();
    RenderedMovableEntity* getBuildingObjectFromTile(Tile* tile);
    /*! \brief Creates a child RenderedMovableEntity mesh using the given mesh name and placing on the target tile,
     *  if the tile is NULL the object appears in the building's center, the rotation angle is given in degrees.
     */
    RenderedMovableEntity* loadBuildingObject(GameMap* gameMap, const std::string& meshName,
        Tile* targetTile, double rotationAngle);
    RenderedMovableEntity* loadBuildingObject(GameMap* gameMap, const std::string& meshName,
        Tile* targetTile, double x, double y, double rotationAngle);
    Tile* getCentralTile();

    virtual bool isAttackable() const;
    virtual void addCoveredTile(Tile* t, double nHP);
    virtual bool removeCoveredTile(Tile* t);
    virtual Tile* getCoveredTile(int index);
    std::vector<Tile*> getCoveredTiles();
    virtual unsigned int numCoveredTiles();
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

protected:
    std::map<Tile*, RenderedMovableEntity*> mBuildingObjects;
    std::vector<Tile*> mCoveredTiles;
    std::map<Tile*, double> mTileHP;

};

#endif // BUILDING_H_
