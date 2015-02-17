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

#ifndef MAPLIGHT_H
#define MAPLIGHT_H

#include "entities/MovableGameEntity.h"

#include <OgrePrerequisites.h>
#include <OgreVector3.h>
#include <OgreColourValue.h>

#include <iostream>
#include <string>

class GameMap;
class ODPacket;

class MapLight: public MovableGameEntity
{
public:
    //! \brief Constructor for making Map lights. If useUniqueName is false, setName() should be called
    MapLight(GameMap*              gameMap,
             Ogre::Real            red         = 0.0,
             Ogre::Real            green       = 0.0,
             Ogre::Real            blue        = 0.0,
             Ogre::Real            range       = 0.0,
             Ogre::Real            constant    = 0.0,
             Ogre::Real            linear      = 0.0,
             Ogre::Real            quadratic   = 0.0);

    virtual ~MapLight()
    {}

    virtual GameEntityType getObjectType() const
    { return GameEntityType::mapLight; }

    inline Ogre::SceneNode* getFlickerNode() const
    { return mFlickerNode; }

    inline void setFlickerNode(Ogre::SceneNode* node)
    { mFlickerNode = node; }

    static const std::string MAPLIGHT_NAME_PREFIX;
    static const std::string MAPLIGHT_INDICATOR_PREFIX;

    virtual std::string getOgreNamePrefix() const
    { return MAPLIGHT_NAME_PREFIX; }

    virtual void addToGameMap();
    virtual void removeFromGameMap();

    virtual void doUpkeep()
    {}

    //! \brief Conform: GameEntity functions handling covered tiles
    std::vector<Tile*> getCoveredTiles();
    Tile* getCoveredTile(int index);
    uint32_t numCoveredTiles();

    virtual double getHP(Tile *tile) const
    { return 0.0; }

    virtual double takeDamage(GameEntity* attacker, double physicalDamage, double magicalDamage, Tile *tileTakingDamage)
    { return 0.0; }

    const Ogre::ColourValue& getDiffuseColor() const
    { return mDiffuseColor; }

    const Ogre::ColourValue& getSpecularColor() const
    { return mSpecularColor; }

    Ogre::Real getAttenuationRange() const
    { return mAttenuationRange; }

    Ogre::Real getAttenuationConstant() const
    { return mAttenuationConstant; }

    Ogre::Real getAttenuationLinear() const
    { return mAttenuationLinear; }

    Ogre::Real getAttenuationQuadratic() const
    { return mAttenuationQuadratic; }

    /** \brief Moves the light in a semi-random fashion around its "native" position.
     * \param time The time variable indicates how much time has elapsed since the last update
     * in seconds.
     */
    void update(Ogre::Real timeSinceLastFrame);

    //! NOTE: If we want to add MapLights on claimed tiles, we should do that on client side
    //! only if possible (and maybe create another class for that that cannot be picked up)
    void notifySeatsWithVision(const std::vector<Seat*>& seats);

    void fireAddEntityToAll();

    virtual bool tryPickup(Seat* seat);
    virtual void pickup();
    virtual bool tryDrop(Seat* seat, Tile* tile);
    virtual void drop(const Ogre::Vector3& v);
    virtual bool canSlap(Seat* seat);
    virtual void slap();

    static MapLight* getMapLightFromStream(GameMap* gameMap, std::istream& is);
    static MapLight* getMapLightFromPacket(GameMap* gameMap, ODPacket& is);

    static std::string getFormat();
    virtual void exportToStream(std::ostream& os) const override;
    virtual void importFromStream(std::istream& is) override;
    virtual void exportToPacket(ODPacket& os) const override;
    virtual void importFromPacket(ODPacket& is) override;

    //! \brief Loads the map light data from a level line.
    static void loadFromLine(const std::string& line, MapLight* m);

protected:
    virtual void createMeshLocal();
    virtual void destroyMeshLocal();
    virtual void fireAddEntity(Seat* seat, bool async);
    virtual void fireRemoveEntity(Seat* seat);

private:
    Ogre::ColourValue mDiffuseColor;
    Ogre::ColourValue mSpecularColor;

    Ogre::Real mAttenuationRange;
    Ogre::Real mAttenuationConstant;
    Ogre::Real mAttenuationLinear;
    Ogre::Real mAttenuationQuadratic;

    Ogre::Real mThetaX;
    Ogre::Real mThetaY;
    Ogre::Real mThetaZ;

    Ogre::Real mFactorX;
    Ogre::Real mFactorY;
    Ogre::Real mFactorZ;

    Ogre::SceneNode* mFlickerNode;
};

#endif // MAPLIGHT_H
