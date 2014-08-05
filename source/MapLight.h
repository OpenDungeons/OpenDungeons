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

#ifndef MAPLIGHT_H
#define MAPLIGHT_H

#include "ODPacket.h"
#include "GameMap.h"

#include <OgrePrerequisites.h>
#include <OgreVector3.h>
#include <OgreColourValue.h>

#include <iostream>
#include <string>

class GameMap;

class MapLight
{
public:
    //! \brief Constructor for making Map lights. If useUniqueName is false, setName() should be called
    MapLight(GameMap*              gameMap,
             bool                  generateUniqueName,
             const Ogre::Vector3&  nPosition   = Ogre::Vector3(0, 0, 0),
             Ogre::Real            red         = 0.0,
             Ogre::Real            green       = 0.0,
             Ogre::Real            blue        = 0.0,
             Ogre::Real            range       = 0.0,
             Ogre::Real            constant    = 0.0,
             Ogre::Real            linear      = 0.0,
             Ogre::Real            quadratic   = 0.0) :
        mOgreEntityExists                (false),
        mOgreEntityVisualIndicatorExists (false),
        mThetaX                          (0.0),
        mThetaY                          (0.0),
        mThetaZ                          (0.0),
        mFactorX                         (0),
        mFactorY                         (0),
        mFactorZ                         (0)
    {
        mGameMap = gameMap;
        if(generateUniqueName)
        {
            std::stringstream tempSS;
            tempSS << "Map_light_ " << mGameMap->nextUniqueNumberMapLight();
            mName = tempSS.str();
        }

        setPosition(nPosition);
        setDiffuseColor(red, green, blue);
        setSpecularColor(red, green, blue);
        setAttenuation(range, constant, linear, quadratic);
    }

    virtual ~MapLight()
    {}

    void setLocation(const Ogre::Vector3& nPosition);
    void setDiffuseColor(Ogre::Real red, Ogre::Real green, Ogre::Real blue);
    void setSpecularColor(Ogre::Real red, Ogre::Real green, Ogre::Real blue);
    void setAttenuation(Ogre::Real range, Ogre::Real constant,
                        Ogre::Real linear, Ogre::Real quadratic);

    void createOgreEntity();
    void destroyOgreEntity();
    void destroyOgreEntityVisualIndicator();
    void deleteYourself();

    void setPosition(Ogre::Real nX, Ogre::Real nY, Ogre::Real nZ);
    void setPosition(const Ogre::Vector3& nPosition);

    const std::string& getName() const
    { return mName; }

    const Ogre::Vector3& getPosition() const
    { return mPosition; }

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
    void advanceFlicker(Ogre::Real time);

    static std::string getFormat();
    friend ODPacket& operator<<(ODPacket& os, MapLight *m);
    friend ODPacket& operator>>(ODPacket& is, MapLight *m);

    //! \brief Loads the map light data from a level line.
    static void loadFromLine(const std::string& line, MapLight* m);

private:
    GameMap* mGameMap;

    Ogre::Vector3 mPosition;

    Ogre::ColourValue mDiffuseColor;
    Ogre::ColourValue mSpecularColor;

    Ogre::Real mAttenuationRange;
    Ogre::Real mAttenuationConstant;
    Ogre::Real mAttenuationLinear;
    Ogre::Real mAttenuationQuadratic;

    //! \brief The entity unique name.
    std::string mName;

    bool mOgreEntityExists;
    bool mOgreEntityVisualIndicatorExists;

    Ogre::Vector3 mFlickerPosition;

    Ogre::Real mThetaX;
    Ogre::Real mThetaY;
    Ogre::Real mThetaZ;

    int mFactorX;
    int mFactorY;
    int mFactorZ;
};

#endif // MAPLIGHT_H
