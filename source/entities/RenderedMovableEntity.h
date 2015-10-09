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
#include <iosfwd>

class Room;
class GameMap;
class Seat;
class ODPacket;

class RenderedMovableEntity: public MovableGameEntity
{
public:
    //! \brief Creates a RenderedMovableEntity. It's name is built from baseName and some unique id from the gamemap.
    //! We use baseName to help understand what's this object for when getting a log
    RenderedMovableEntity(GameMap* gameMap, bool isOnServerMap, const std::string& baseName, const std::string& nMeshName,
        Ogre::Real rotationAngle, bool hideCoveredTile, float opacity = 1.0f);
    RenderedMovableEntity(GameMap* gameMap, bool isOnServerMap);

    static const std::string RENDEREDMOVABLEENTITY_PREFIX;

    virtual void addToGameMap() override;
    virtual void removeFromGameMap() override;

    bool getHideCoveredTile() const
    { return mHideCoveredTile; }

    virtual void doUpkeep() override
    {}

    virtual const Ogre::Vector3& getScale() const override;

    void receiveExp(double experience)
    {}

    double takeDamage(GameEntity* attacker, double physicalDamage, double magicalDamage, double elementDamage,
        Tile *tileTakingDamage, bool ignorePhysicalDefense, bool ignoreMagicalDefense, bool ignoreElementDefense) override
    { return 0.0; }

    double getHP(Tile *tile) const override
    { return 0; }

    //! \brief Conform: GameEntity functions handling covered tiles
    std::vector<Tile*> getCoveredTiles() override;
    Tile* getCoveredTile(int index) override;
    uint32_t numCoveredTiles() override;

    Ogre::Real getRotationAngle() const
    { return mRotationAngle; }

    inline float getOpacity() const
    { return mOpacity; }

    virtual void setMeshOpacity(float opacity);

    virtual void pickup() override;
    virtual void drop(const Ogre::Vector3& v) override;

    //! Notify the RenderedMovableEntity that it is asked to be removed. If it returns
    //! true, it can be removed. Otherwise, that means that it should not. That allows
    //! to use PersistentObjects that are visible even when vision is lost.
    virtual bool notifyRemoveAsked()
    { return true; }

    static std::string getRenderedMovableEntityStreamFormat();

protected:
    //! \brief Exports the data of the RenderedMovableEntity
    virtual void exportToStream(std::ostream& os) const override;
    virtual bool importFromStream(std::istream& is) override;
    virtual void exportToPacket(ODPacket& os, const Seat* seat) const override;
    virtual void importFromPacket(ODPacket& is) override;

    virtual void createMeshLocal() override;
    virtual void destroyMeshLocal() override;
    virtual void fireAddEntity(Seat* seat, bool async) override;
    virtual void fireRemoveEntity(Seat* seat) override;

    std::string mBaseName;

private:
    Ogre::Real mRotationAngle;
    bool mHideCoveredTile;

    //! \brief The model current opacity
    float mOpacity;
};

#endif // RENDEREDMOVABLEENTITY_H
