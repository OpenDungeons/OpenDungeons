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

#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "render/RenderManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
//#include "utils/Random.h"

#include <istream>
#include <ostream>

const std::string MapLight::MAPLIGHT_NAME_PREFIX = "Map_light_";

MapLight::MapLight(GameMap* gameMap, bool isOnServerMap, Ogre::Real diffRed, Ogre::Real diffGreen, Ogre::Real diffBlue,
        Ogre::Real specRed, Ogre::Real specGreen, Ogre::Real specBlue,
        Ogre::Real attenRange, Ogre::Real attenConst, Ogre::Real attenLin, Ogre::Real attenQuad) :
    MovableGameEntity                   (gameMap, isOnServerMap),
    mThetaX                             (0.0),
    mThetaY                             (0.0),
    mThetaZ                             (0.0),
    mFactorX                            (0.0),
    mFactorY                            (0.0),
    mFactorZ                            (0.0),
    mFlickerNode                        (nullptr)
{
    mDiffuseColor = Ogre::ColourValue(diffRed, diffGreen, diffBlue);
    mSpecularColor = Ogre::ColourValue(specRed, specGreen, specBlue);
    mAttenuationRange = attenRange;
    mAttenuationConstant = attenConst;
    mAttenuationLinear = attenLin;
    mAttenuationQuadratic = attenQuad;
}

void MapLight::createMeshLocal()
{
    MovableGameEntity::createMeshLocal();

    if(getIsOnServerMap())
        return;

    // Only show the visual light entity if we are in editor mode
    RenderManager::getSingleton().rrCreateMapLight(this, getGameMap()->isInEditorMode());
}

void MapLight::destroyMeshLocal()
{
    MovableGameEntity::destroyMeshLocal();

    if(getIsOnServerMap())
        return;

    RenderManager::getSingleton().rrDestroyMapLightVisualIndicator(this);

    RenderManager::getSingleton().rrDestroyMapLight(this);
}

void MapLight::addToGameMap()
{
    getGameMap()->addMapLight(this);
    getGameMap()->addAnimatedObject(this);
}

void MapLight::removeFromGameMap()
{
    fireEntityRemoveFromGameMap();
    removeEntityFromPositionTile();
    getGameMap()->removeMapLight(this);
    getGameMap()->removeAnimatedObject(this);


    if(!getIsOnServerMap())
        return;

    fireRemoveEntityToSeatsWithVision();
}

std::vector<Tile*> MapLight::getCoveredTiles()
{
    std::vector<Tile*> tempVector;
    tempVector.push_back(getPositionTile());
    return tempVector;
}

Tile* MapLight::getCoveredTile(int index)
{
    if(index > 0)
    {
        OD_LOG_ERR("name=" + getName() + ", index=" + Helper::toString(index));
        return nullptr;
    }

    return getPositionTile();
}

uint32_t MapLight::numCoveredTiles()
{
    if(getPositionTile() == nullptr)
        return 0;

    return 1;
}

void MapLight::update(Ogre::Real timeSinceLastFrame)
{
  //Disabled for now.
/*    if(getIsOnServerMap())
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
    RenderManager::getSingleton().rrMoveMapLightFlicker(this, flickerPosition);*/
}

void MapLight::fireAddEntity(Seat* seat, bool async)
{
    if(async)
    {
        ServerNotification serverNotification(
            ServerNotificationType::addEntity, seat->getPlayer());
        exportHeadersToPacket(serverNotification.mPacket);
        exportToPacket(serverNotification.mPacket, seat);
        ODServer::getSingleton().sendAsyncMsg(serverNotification);
    }
    else
    {
        ServerNotification* serverNotification = new ServerNotification(
            ServerNotificationType::addEntity, seat->getPlayer());
        exportHeadersToPacket(serverNotification->mPacket);
        exportToPacket(serverNotification->mPacket, seat);
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

void MapLight::fireRemoveEntity(Seat* seat)
{
    const std::string& name = getName();
    ServerNotification *serverNotification = new ServerNotification(
        ServerNotificationType::removeEntity, seat->getPlayer());
    GameEntityType type = getObjectType();
    serverNotification->mPacket << type;
    serverNotification->mPacket << name;
    ODServer::getSingleton().queueServerNotification(serverNotification);
}

void MapLight::notifySeatsWithVision(const std::vector<Seat*>& seats)
{
    if(!mSeatsWithVisionNotified.empty())
        return;

    // MapLights are visible for every seat and are not removed when vision is lost
    const std::vector<Seat*>& allSeats = getGameMap()->getSeats();
    // We notify seats that gain vision
    for(Seat* seat : allSeats)
    {
        mSeatsWithVisionNotified.push_back(seat);

        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        fireAddEntity(seat, false);
    }
}

bool MapLight::tryPickup(Seat* seat)
{
    if(!getIsOnMap())
        return false;

    // We can pickup MapLights only in editor mode
    if(getGameMap()->isInEditorMode())
        return true;

    return false;
}

void MapLight::pickup()
{
    removeEntityFromPositionTile();
}

bool MapLight::tryDrop(Seat* seat, Tile* tile)
{
    // In editor mode, we allow to drop the MapLight on any tile
    if(getGameMap()->isInEditorMode())
        return true;

    return false;
}

void MapLight::drop(const Ogre::Vector3& v)
{
    setPosition(v);
}

bool MapLight::canSlap(Seat* seat)
{
    if(!getIsOnMap())
        return false;

    // In editor mode, we allow to slap the MapLight to destroy it
    if(getGameMap()->isInEditorMode())
        return true;

    return false;
}

void MapLight::slap()
{
    if(!getIsOnServerMap())
        return;

    if(!getIsOnMap())
        return;

    // In editor mode, we allow to slap the MapLight to destroy it
    if(!getGameMap()->isInEditorMode())
        return;

    removeFromGameMap();
    deleteYourself();
    return;
}

std::string MapLight::getMapLightStreamFormat()
{
    return "posX\tposY\tposZ\tdiffuseR\tdiffuseG\tdiffuseB\tspecularR\tspecularG\tspecularB\tattenRange\tattenConst\tattenLin\tattenQuad";
}

MapLight* MapLight::getMapLightFromStream(GameMap* gameMap, std::istream& is)
{
    MapLight* mapLight = new MapLight(gameMap, true);
    mapLight->importFromStream(is);
    return mapLight;
}

MapLight* MapLight::getMapLightFromPacket(GameMap* gameMap, ODPacket& is)
{
    MapLight* mapLight = new MapLight(gameMap, false);
    mapLight->importFromPacket(is);
    return mapLight;
}

void MapLight::exportToStream(std::ostream& os) const
{
    os << mPosition.x << "\t" << mPosition.y << "\t" << mPosition.z;
    os << "\t" << mDiffuseColor.r << "\t" << mDiffuseColor.g << "\t" << mDiffuseColor.b;
    os << "\t" << mSpecularColor.r << "\t" << mSpecularColor.g << "\t" << mSpecularColor.b;
    os << "\t" << mAttenuationRange << "\t" << mAttenuationConstant;
    os << "\t" << mAttenuationLinear << "\t" << mAttenuationQuadratic;
}

void MapLight::importFromStream(std::istream& is)
{
    OD_ASSERT_TRUE(is >> mPosition.x >> mPosition.y >> mPosition.z);
    OD_ASSERT_TRUE(is >> mDiffuseColor.r >> mDiffuseColor.g >> mDiffuseColor.b);
    OD_ASSERT_TRUE(is >> mSpecularColor.r >> mSpecularColor.g >> mSpecularColor.b);
    OD_ASSERT_TRUE(is >> mAttenuationRange >> mAttenuationConstant);
    OD_ASSERT_TRUE(is >> mAttenuationLinear >> mAttenuationQuadratic);
}

void MapLight::exportToPacket(ODPacket& os, const Seat* seat) const
{
    const std::string& name = getName();
    os << name;
    os << mPosition.x << mPosition.y << mPosition.z;
    os << mDiffuseColor.r << mDiffuseColor.g << mDiffuseColor.b;
    os << mSpecularColor.r << mSpecularColor.g << mSpecularColor.b;
    os << mAttenuationRange << mAttenuationConstant;
    os << mAttenuationLinear << mAttenuationQuadratic;
}

void MapLight::importFromPacket(ODPacket& is)
{
    std::string name;
    OD_ASSERT_TRUE(is >> name);
    setName(name);
    OD_ASSERT_TRUE(is >> mPosition.x >> mPosition.y >> mPosition.z);
    OD_ASSERT_TRUE(is >> mDiffuseColor.r >> mDiffuseColor.g >> mDiffuseColor.b);
    OD_ASSERT_TRUE(is >> mSpecularColor.r >> mSpecularColor.g >> mSpecularColor.b);
    OD_ASSERT_TRUE(is >> mAttenuationRange >> mAttenuationConstant);
    OD_ASSERT_TRUE(is >> mAttenuationLinear >> mAttenuationQuadratic);
}
