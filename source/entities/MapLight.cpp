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

#include "entities/MapLight.h"

#include "gamemap/GameMap.h"
#include "modes/ModeManager.h"
#include "network/ODPacket.h"
#include "render/RenderManager.h"
#include "render/ODFrameListener.h"
#include "utils/Helper.h"
#include "utils/Random.h"

#include <sstream>

const std::string MapLight::MAPLIGHT_NAME_PREFIX = "Map_light_";
const std::string MapLight::MAPLIGHT_INDICATOR_PREFIX = "MapLightIndicator_";

MapLight::MapLight(GameMap* gameMap, Ogre::Real red, Ogre::Real green, Ogre::Real blue,
        Ogre::Real range, Ogre::Real constant, Ogre::Real linear, Ogre::Real quadratic) :
    MovableGameEntity                   (gameMap),
    mThetaX                             (0.0),
    mThetaY                             (0.0),
    mThetaZ                             (0.0),
    mFactorX                            (0.0),
    mFactorY                            (0.0),
    mFactorZ                            (0.0),
    mFlickerNode                        (nullptr)
{
    mDiffuseColor = Ogre::ColourValue(red, green, blue);
    mSpecularColor = Ogre::ColourValue(red, green, blue);
    mAttenuationRange = range;
    mAttenuationConstant = constant;
    mAttenuationLinear = linear;
    mAttenuationQuadratic = quadratic;
}

void MapLight::createMeshLocal()
{
    MovableGameEntity::createMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    // Only show the visual light entity if we are in editor mode
    RenderManager::getSingleton().rrCreateMapLight(this, mm && mm->getCurrentModeType() == ModeManager::EDITOR);
}

void MapLight::destroyMeshLocal()
{
    MovableGameEntity::destroyMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    RenderManager::getSingleton().rrDestroyMapLightVisualIndicator(this);

    RenderManager::getSingleton().rrDestroyMapLight(this);
}

void MapLight::update(Ogre::Real timeSinceLastFrame)
{
    if(getGameMap()->isServerGameMap())
        return;

    mThetaX += static_cast<Ogre::Real>(mFactorX * 3.0 * timeSinceLastFrame);
    mThetaY += static_cast<Ogre::Real>(mFactorY * 3.0 * timeSinceLastFrame);
    mThetaZ += static_cast<Ogre::Real>(mFactorZ * 3.0 * timeSinceLastFrame);

    if (Random::Double(0.0, 1.0) < 0.1)
        mFactorX *= -1.0;
    if (Random::Double(0.0, 1.0) < 0.1)
        mFactorY *= -1.0;
    if (Random::Double(0.0, 1.0) < 0.1)
        mFactorZ *= -1.0;

    Ogre::Vector3 flickerPosition = Ogre::Vector3(sin(mThetaX), sin(mThetaY), sin(mThetaZ));
    RenderManager::getSingleton().rrMoveMapLightFlicker(this, flickerPosition);
}

std::string MapLight::getFormat()
{
    return "posX\tposY\tposZ\tdiffuseR\tdiffuseG\tdiffuseB\tspecularR\tspecularG\tspecularB\tattenRange\tattenConst\tattenLin\tattenQuad";
}

ODPacket& operator<<(ODPacket& os, MapLight* m)
{
    const std::string& name = m->getName();
    os << name;
    os << m->mPosition.x << m->mPosition.y << m->mPosition.z;
    os << m->mDiffuseColor.r << m->mDiffuseColor.g << m->mDiffuseColor.b;
    os << m->mSpecularColor.r << m->mSpecularColor.g << m->mSpecularColor.b;
    os << m->mAttenuationRange << m->mAttenuationConstant;
    os << m->mAttenuationLinear << m->mAttenuationQuadratic;

    return os;
}

ODPacket& operator>>(ODPacket& is, MapLight* m)
{
    std::string name;
    is >> name;
    m->setName(name);
    is >> m->mPosition.x >> m->mPosition.y >> m->mPosition.z;
    is >> m->mDiffuseColor.r >> m->mDiffuseColor.g >> m->mDiffuseColor.b;
    is >> m->mSpecularColor.r >> m->mSpecularColor.g >> m->mSpecularColor.b;
    is >> m->mAttenuationRange >> m->mAttenuationConstant;
    is >> m->mAttenuationLinear >> m->mAttenuationQuadratic;

    return is;
}

void MapLight::loadFromLine(const std::string& line, MapLight* m)
{
    std::vector<std::string> elems = Helper::split(line, '\t');

    m->mPosition.x = Helper::toDouble(elems[0]);
    m->mPosition.y = Helper::toDouble(elems[1]);
    m->mPosition.z = Helper::toDouble(elems[2]);

    m->mDiffuseColor.r = Helper::toDouble(elems[3]);
    m->mDiffuseColor.g = Helper::toDouble(elems[4]);
    m->mDiffuseColor.b = Helper::toDouble(elems[5]);

    m->mSpecularColor.r = Helper::toDouble(elems[6]);
    m->mSpecularColor.g = Helper::toDouble(elems[7]);
    m->mSpecularColor.b = Helper::toDouble(elems[8]);

    m->mAttenuationRange = Helper::toDouble(elems[9]);
    m->mAttenuationConstant = Helper::toDouble(elems[10]);
    m->mAttenuationLinear = Helper::toDouble(elems[11]);
    m->mAttenuationQuadratic = Helper::toDouble(elems[12]);
}

std::ostream& operator<<(std::ostream& os, MapLight *m)
{
    os << m->mPosition.x << "\t" << m->mPosition.y << "\t" << m->mPosition.z;
    os << "\t" << m->mDiffuseColor.r << "\t" << m->mDiffuseColor.g << "\t" << m->mDiffuseColor.b;
    os << "\t" << m->mSpecularColor.r << "\t" << m->mSpecularColor.g << "\t" << m->mSpecularColor.b;
    os << "\t" << m->mAttenuationRange << "\t" << m->mAttenuationConstant;
    os << "\t" << m->mAttenuationLinear << "\t" << m->mAttenuationQuadratic;
    return os;
}

std::istream& operator>>(std::istream& is, MapLight *m)
{
    is >> m->mPosition.x >> m->mPosition.y >> m->mPosition.z;
    is >> m->mDiffuseColor.r >> m->mDiffuseColor.g >> m->mDiffuseColor.b;
    is >> m->mSpecularColor.r >> m->mSpecularColor.g >> m->mSpecularColor.b;
    is >> m->mAttenuationRange >> m->mAttenuationConstant;
    is >> m->mAttenuationLinear >> m->mAttenuationQuadratic;
    return is;
}
