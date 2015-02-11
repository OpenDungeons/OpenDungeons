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

enum class RenderedMovableEntityType
{
    buildingObject,
    treasuryObject,
    chickenEntity,
    smallSpiderEntity,
    craftedTrap,
    missileObject,
    persistentObject,
    trapEntity,
    spellEntity
};

ODPacket& operator<<(ODPacket& os, const RenderedMovableEntityType& rot);
ODPacket& operator>>(ODPacket& is, RenderedMovableEntityType& rot);
std::ostream& operator<<(std::ostream& os, const RenderedMovableEntityType& rot);
std::istream& operator>>(std::istream& is, RenderedMovableEntityType& rot);

class RenderedMovableEntity: public MovableGameEntity
{
public:
    //! \brief Creates a RenderedMovableEntity. It's name is built from baseName and some unique id from the gamemap.
    //! We use baseName to help understand what's this object for when getting a log
    RenderedMovableEntity(GameMap* gameMap, const std::string& baseName, const std::string& nMeshName,
        Ogre::Real rotationAngle, bool hideCoveredTile, float opacity = 1.0f, const std::string& initialAnimationState = "",
        bool initialAnimationLoop = true);
    RenderedMovableEntity(GameMap* gameMap);

    virtual GameEntityType getObjectType() const
    { return GameEntityType::renderedMovableEntity; }

    static const std::string RENDEREDMOVABLEENTITY_PREFIX;
    static const std::string RENDEREDMOVABLEENTITY_OGRE_PREFIX;

    virtual std::string getOgreNamePrefix() const { return RENDEREDMOVABLEENTITY_OGRE_PREFIX; }

    virtual void addToGameMap();
    virtual void removeFromGameMap();

    bool getHideCoveredTile() const
    { return mHideCoveredTile; }

    virtual void doUpkeep()
    {}

    virtual const Ogre::Vector3& getScale() const;

    void receiveExp(double experience)
    {}

    double takeDamage(GameEntity* attacker, double physicalDamage, double magicalDamage, Tile* tileTakingDamage)
    { return 0.0; }

    double getHP(Tile *tile) const
    { return 0; }

    //! \brief Conform: GameEntity functions handling covered tiles
    std::vector<Tile*> getCoveredTiles();
    Tile* getCoveredTile(int index);
    uint32_t numCoveredTiles();

    Ogre::Real getRotationAngle() const
    { return mRotationAngle; }

    inline float getOpacity() const
    { return mOpacity; }

    virtual void setMeshOpacity(float opacity);

    virtual RenderedMovableEntityType getRenderedMovableEntityType() const = 0;

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
    virtual void exportToStream(std::ostream& os) const;
    virtual void importFromStream(std::istream& is);
    virtual void exportToPacket(ODPacket& os) const;
    virtual void importFromPacket(ODPacket& is);

    static RenderedMovableEntity* getRenderedMovableEntityFromLine(GameMap* gameMap, const std::string& line);
    static RenderedMovableEntity* getRenderedMovableEntityFromPacket(GameMap* gameMap, ODPacket& is);
    static const char* getFormat();

protected:
    virtual void createMeshLocal();
    virtual void destroyMeshLocal();
    virtual void fireAddEntity(Seat* seat, bool async);
    virtual void fireRemoveEntity(Seat* seat);
private:
    Ogre::Real mRotationAngle;
    bool mHideCoveredTile;

    //! \brief The model current opacity
    float mOpacity;
};

#endif // RENDEREDMOVABLEENTITY_H
