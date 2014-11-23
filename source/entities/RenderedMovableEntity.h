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

#ifndef RENDEREDMOVABLEENTITY_H
#define RENDEREDMOVABLEENTITY_H

#include "entities/MovableGameEntity.h"

#include <string>
#include <istream>
#include <ostream>

class Room;
class GameMap;
class Seat;
class ODPacket;

class RenderedMovableEntity: public MovableGameEntity
{
public:
    enum RenderedMovableEntityType
    {
        buildingObject,
        treasuryObject,
        chickenEntity,
        smallSpiderEntity,
        missileObject
    };
    //! \brief Creates a RenderedMovableEntity. It's name is built from baseName and some unique id from the gamemap.
    //! We use baseName to help understand what's this object for when getting a log
    RenderedMovableEntity(GameMap* gameMap, const std::string& baseName, const std::string& nMeshName,
        Ogre::Real rotationAngle, bool hideCoveredTile);
    RenderedMovableEntity(GameMap* gameMap);

    static const std::string RENDEREDMOVABLEENTITY_PREFIX;
    static const std::string RENDEREDMOVABLEENTITY_OGRE_PREFIX;

    virtual std::string getOgreNamePrefix() const { return RENDEREDMOVABLEENTITY_OGRE_PREFIX; }

    bool getHideCoveredTile() const
    { return mHideCoveredTile; }

    virtual void doUpkeep()
    {}

    void receiveExp(double experience)
    {}

    double takeDamage(GameEntity* attacker, double physicalDamage, double magicalDamage, Tile* tileTakingDamage)
    { return 0.0; }

    double getHP(Tile *tile) const
    { return 0; }

    std::vector<Tile*> getCoveredTiles()
    { return std::vector<Tile*>(); }

    Ogre::Real getRotationAngle()
    { return mRotationAngle; }

    virtual RenderedMovableEntityType getRenderedMovableEntityType()
    { return RenderedMovableEntityType::buildingObject; }

    virtual bool tryPickup(Seat* seat, bool isEditorMode)
    { return false; }

    virtual bool tryDrop(Seat* seat, Tile* tile, bool isEditorMode)
    { return false; }

    virtual void pickup();
    virtual void drop(const Ogre::Vector3& v);

    /*! \brief Exports the headers needed to recreate the RenderedMovableEntity. For example, for
     * missile objects type cannon, it exports RenderedMovableEntity::missileObject
     * and MissileType::oneHit. The content of the RenderedMovableEntity will be exported
     * by exportToPacket
     */
    virtual void exportHeadersToStream(std::ostream& os);
    virtual void exportHeadersToPacket(ODPacket& os);
    //! \brief Exports the data of the RenderedMovableEntity
    virtual void exportToStream(std::ostream& os);
    virtual void importFromStream(std::istream& is);
    virtual void exportToPacket(ODPacket& os);
    virtual void importFromPacket(ODPacket& is);

    static RenderedMovableEntity* getRenderedMovableEntityFromLine(GameMap* gameMap, const std::string& line);
    static RenderedMovableEntity* getRenderedMovableEntityFromPacket(GameMap* gameMap, ODPacket& is);
    static const char* getFormat();

    friend ODPacket& operator<<(ODPacket& os, const RenderedMovableEntity::RenderedMovableEntityType& rot);
    friend ODPacket& operator>>(ODPacket& is, RenderedMovableEntity::RenderedMovableEntityType& rot);
    friend std::ostream& operator<<(std::ostream& os, const RenderedMovableEntity::RenderedMovableEntityType& rot);
    friend std::istream& operator>>(std::istream& is, RenderedMovableEntity::RenderedMovableEntityType& rot);

protected:
    virtual bool getIsOnMap()
    { return mIsOnMap; }

    virtual void createMeshLocal();
    virtual void destroyMeshLocal();
private:
    Ogre::Real mRotationAngle;
    bool mIsOnMap;
    bool mHideCoveredTile;
};

#endif // RENDEREDMOVABLEENTITY_H
