/*!
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

#include "gamemap/MiniMapCamera.h"

#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "render/ODFrameListener.h"
#include "render/RenderManager.h"
#include "utils/LogManager.h"

#include <OgrePrerequisites.h>
#include <OgreSceneManager.h>
#include <OgreTextureManager.h>

#include <CEGUI/BasicImage.h>
#include <CEGUI/Image.h>
#include <CEGUI/ImageManager.h>
#include <CEGUI/PropertyHelper.h>
#include <CEGUI/RendererModules/Ogre/Renderer.h>
#include <CEGUI/Size.h>
#include <CEGUI/System.h>
#include <CEGUI/Texture.h>
#include <CEGUI/Window.h>
#include <CEGUI/WindowManager.h>

static const Ogre::uint TEXTURE_SIZE = 512;
static const Ogre::Real MIN_TIME_REFRESH_SECS = 0.5;

MiniMapCamera::MiniMapCamera(CEGUI::Window* miniMapWindow) :
    mMiniMapWindow(miniMapWindow),
    mTopLeftCornerX(0),
    mTopLeftCornerY(0),
    mWidth(0),
    mHeight(0),
    mMapX(ODFrameListener::getSingleton().getClientGameMap()->getMapSizeX()),
    mMapY(ODFrameListener::getSingleton().getClientGameMap()->getMapSizeY()),
    mElapsedTime(MIN_TIME_REFRESH_SECS),
    mMiniMapOgreTexture(Ogre::TextureManager::getSingletonPtr()->createManual(
            "miniMapOgreTexture",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::TEX_TYPE_2D,
            TEXTURE_SIZE, TEXTURE_SIZE, 0, Ogre::PF_R8G8B8,
            Ogre::TU_RENDERTARGET))
{
    // We create a special camera that will look at the whole scene and render it in the minimap
    Ogre::SceneManager* sceneManager = RenderManager::getSingleton().getSceneManager();
    Ogre::Camera* miniMapCam = sceneManager->createCamera("miniMapCam");
    miniMapCam->setNearClipDistance(0.02);
    miniMapCam->setFarClipDistance(300.0);
    Ogre::Radian angle(Ogre::Degree(90.0));
    miniMapCam->setFOVy(angle);
    Ogre::Real tan = Ogre::Math::Tan(angle * 0.5);
    Ogre::Real mapHighestSize = std::max(mMapX, mMapY);
    Ogre::Real camHeight = mapHighestSize * 0.5 / tan;

    Ogre::RenderTarget* rt = mMiniMapOgreTexture->getBuffer()->getRenderTarget();
    rt->setAutoUpdated(false);
    miniMapCam->setAspectRatio(1.0);
    miniMapCam->setPosition(mMapX / 2.0, mMapY / 2.0, camHeight);
    miniMapCam->lookAt(mMapX / 2.0, mMapY / 2.0, 0.0);
    Ogre::Viewport* vp = rt->addViewport(miniMapCam);
    vp->setClearEveryFrame(true);
    vp->setOverlaysEnabled(false);
    vp->setBackgroundColour(Ogre::ColourValue::Black);

    CEGUI::Texture& miniMapTextureGui = static_cast<CEGUI::OgreRenderer*>(CEGUI::System::getSingletonPtr()
                                            ->getRenderer())->createTexture("miniMapTextureGui", mMiniMapOgreTexture);

    CEGUI::BasicImage& imageset = dynamic_cast<CEGUI::BasicImage&>(CEGUI::ImageManager::getSingletonPtr()->create("BasicImage", "MiniMapImageset"));
    imageset.setArea(CEGUI::Rectf(CEGUI::Vector2f(0.0, 0.0),
        CEGUI::Size<float>(static_cast<float>(TEXTURE_SIZE), static_cast<float>(TEXTURE_SIZE))));

    // Link the image to the minimap
    imageset.setTexture(&miniMapTextureGui);
    mMiniMapWindow->setProperty("Image", CEGUI::PropertyHelper<CEGUI::Image*>::toString(&imageset));

//    mMiniMapOgreTexture->load();

    mTopLeftCornerX = mMiniMapWindow->getUnclippedOuterRect().get().getPosition().d_x;
    mTopLeftCornerY = mMiniMapWindow->getUnclippedOuterRect().get().getPosition().d_y;
    mWidth = mMiniMapWindow->getUnclippedOuterRect().get().getSize().d_width;
    mHeight = mMiniMapWindow->getUnclippedOuterRect().get().getSize().d_height;
}

MiniMapCamera::~MiniMapCamera()
{
    mMiniMapWindow->setProperty("Image", "");
    Ogre::SceneManager* sceneManager = RenderManager::getSingleton().getSceneManager();
    sceneManager->destroyCamera("miniMapCam");
    Ogre::TextureManager::getSingletonPtr()->remove("miniMapOgreTexture");
    CEGUI::ImageManager::getSingletonPtr()->destroy("MiniMapImageset");
    CEGUI::System::getSingletonPtr()->getRenderer()->destroyTexture("miniMapTextureGui");
}

Ogre::Vector2 MiniMapCamera::camera_2dPositionFromClick(int xx, int yy)
{
    // Compute tile clicked
    Ogre::Real coordsX;
    if(mMapX < mMapY)
        coordsX = ((xx - mTopLeftCornerX) * mMapY / mWidth) - (mMapY - mMapX) / 2.0;
    else
        coordsX = (xx - mTopLeftCornerX) * mMapX / mWidth;

    Ogre::Real coordsY;
    if(mMapY < mMapX)
        coordsY = (mMapX - ((yy - mTopLeftCornerY) * mMapX / mHeight)) - (mMapX - mMapY) / 2.0;
    else
        coordsY = mMapY - ((yy - mTopLeftCornerY) * mMapY / mHeight);

    Ogre::Vector2 pos(coordsX, coordsY);
    return pos;
}

void MiniMapCamera::update(Ogre::Real timeSinceLastFrame)
{
    mElapsedTime += timeSinceLastFrame;
    if(mElapsedTime < MIN_TIME_REFRESH_SECS)
        return;

    mElapsedTime -= MIN_TIME_REFRESH_SECS;
    Ogre::RenderTarget* rt = mMiniMapOgreTexture->getBuffer()->getRenderTarget();
    rt->update();
}
