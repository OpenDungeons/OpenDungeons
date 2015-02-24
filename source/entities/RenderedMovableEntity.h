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

class RenderedMovableEntity: public MovableGameEntity
{
public:
    //! \brief Creates a RenderedMovableEntity. It's name is built from baseName and some unique id from the gamemap.
    //! We use baseName to help understand what's this object for when getting a log
    RenderedMovableEntity(GameMap* gameMap, const std::string& baseName, const std::string& nMeshName,
        Ogre::Real rotationAngle, bool hideCoveredTile, float opacity = 1.0f, const std::string& initialAnimationState = "",
        bool initialAnimationLoop = true);
    RenderedMovableEntity(GameMap* gameMap);

    static const std::string RENDEREDMOVABLEENTITY_PREFIX;

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

    virtual void pickup();
    virtual void drop(const Ogre::Vector3& v);

    //! \brief Exports the data of the RenderedMovableEntity
    virtual void exportToStream(std::ostream& os) const override;
    virtual void importFromStream(std::istream& is) override;
    virtual void exportToPacket(ODPacket& os) const override;
    virtual void importFromPacket(ODPacket& is) override;

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
