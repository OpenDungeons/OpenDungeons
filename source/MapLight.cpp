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

#include "MapLight.h"

#include "RenderManager.h"
#include "RenderRequest.h"
#include "GameMap.h"
#include "Random.h"
#include "ODClient.h"
#include "ODServer.h"

#include "Helper.h"

#include <sstream>

void MapLight::setLocation(const Ogre::Vector3& nPosition)
{
    //TODO: This needs to make a RenderRequest to actually move the light.
    mPosition = nPosition;
}

void MapLight::setDiffuseColor(Ogre::Real red, Ogre::Real green, Ogre::Real blue)
{
    mDiffuseColor = Ogre::ColourValue(red, green, blue);
    //TODO: Call refresh of the OGRE entity.
}

void MapLight::setSpecularColor(Ogre::Real red, Ogre::Real green, Ogre::Real blue)
{
    mSpecularColor = Ogre::ColourValue(red, green, blue);
    //TODO: Call refresh of the OGRE entity.
}

void MapLight::setAttenuation(Ogre::Real range, Ogre::Real constant,
                              Ogre::Real linear, Ogre::Real quadratic)
{
    mAttenuationRange = range;
    mAttenuationConstant = constant;
    mAttenuationLinear = linear;
    mAttenuationQuadratic = quadratic;
    //TODO: Call refresh of the OGRE entity.
}

void MapLight::createOgreEntity()
{
    if (mOgreEntityExists)
        return;

    mOgreEntityExists = true;

    if(mGameMap->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::createMapLight;
    //TODO - this check should be put somewhere else / fixed
    request->p = static_cast<void*>(this);
    request->b = (!ODServer::getSingleton().isConnected() && !ODClient::getSingleton().isConnected());

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);

    mOgreEntityVisualIndicatorExists = true;
}

void MapLight::destroyOgreEntity()
{
    if (!mOgreEntityExists)
        return;

    mOgreEntityExists = false;

    if(mGameMap->isServerGameMap())
        return;

    destroyOgreEntityVisualIndicator();

    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::destroyMapLight;
    request->p = this;
    /* Check if we are in editor mode
     * TODO this check should be elsewhere / fixed
     */
    request->b = (!ODServer::getSingleton().isConnected() && !ODClient::getSingleton().isConnected());

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}

void MapLight::destroyOgreEntityVisualIndicator()
{
    if (!mOgreEntityExists || !mOgreEntityVisualIndicatorExists)
        return;

    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::destroyMapLightVisualIndicator;
    request->p = this;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);

    mOgreEntityVisualIndicatorExists = false;
}

void MapLight::deleteYourself()
{
    destroyOgreEntity();

    if(mGameMap->isServerGameMap())
    {
        mGameMap->queueMapLightForDeletion(this);
        return;
    }

    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::deleteMapLight;
    request->p = this;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}

void MapLight::setPosition(Ogre::Real nX, Ogre::Real nY, Ogre::Real nZ)
{
    setPosition(Ogre::Vector3(nX, nY, nZ));
}

void MapLight::setPosition(const Ogre::Vector3& nPosition)
{
    mPosition = nPosition;

    // Create a RenderRequest to notify the render queue that the scene node for this creature needs to be moved.
    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::moveSceneNode;
    request->str = getOgreNamePrefix() + mName + "_node";
    request->vec = mPosition;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}

void MapLight::advanceFlicker(Ogre::Real time)
{

    mThetaX += (Ogre::Real)(mFactorX * 3.14 * time);
    mThetaY += (Ogre::Real)(mFactorY * 3.14 * time);
    mThetaZ += (Ogre::Real)(mFactorZ * 3.14 * time);

    if (Random::Double(0.0, 1.0) < 0.1)
        mFactorX *= -1;
    if (Random::Double(0.0, 1.0) < 0.1)
        mFactorY *= -1;
    if (Random::Double(0.0, 1.0) < 0.1)
        mFactorZ *= -1;

    mFlickerPosition = Ogre::Vector3(sin(mThetaX), sin(mThetaY), sin(mThetaZ));

    // Create a RenderRequest to notify the render queue that the scene node for this creature needs to be moved.
    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::moveSceneNode;
    request->str = getOgreNamePrefix() + mName + "_flicker_node";
    request->vec = mFlickerPosition;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}

std::string MapLight::getFormat()
{
    return "posX\tposY\tposZ\tdiffuseR\tdiffuseG\tdiffuseB\tspecularR\tspecularG\tspecularB\tattenRange\tattenConst\tattenLin\tattenQuad";
}

ODPacket& operator<<(ODPacket& os, MapLight* m)
{
    os << m->mPosition.x << m->mPosition.y << m->mPosition.z;
    os << m->mDiffuseColor.r << m->mDiffuseColor.g << m->mDiffuseColor.b;
    os << m->mSpecularColor.r << m->mSpecularColor.g << m->mSpecularColor.b;
    os << m->mAttenuationRange << m->mAttenuationConstant;
    os << m->mAttenuationLinear << m->mAttenuationQuadratic;

    return os;
}

ODPacket& operator>>(ODPacket& is, MapLight* m)
{
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
