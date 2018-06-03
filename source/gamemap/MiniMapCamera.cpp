/*!
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#include "camera/CullingManager.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "render/ODFrameListener.h"
#include "render/RenderManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <OgreCamera.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgrePrerequisites.h>
#include <OgreRenderTexture.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreTextureManager.h>
#include <OgreViewport.h>

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

static const Ogre::Real NB_TILES_DISPLAYED_IN_MINIMAP = 30.0;
static const Ogre::Radian ANGLE_CAM = Ogre::Degree(90.0);
static const Ogre::Real CAM_HEIGHT = NB_TILES_DISPLAYED_IN_MINIMAP * 0.5f / Ogre::Math::Tan(ANGLE_CAM * 0.5f);
static const Ogre::uint TEXTURE_SIZE = 512;
static const Ogre::Real MIN_TIME_REFRESH_SECS = 0.5;

MiniMapCamera::MiniMapCamera(CEGUI::Window* miniMapWindow) :
    mMiniMapWindow(miniMapWindow),
    mGameMap(*ODFrameListener::getSingleton().getClientGameMap()),
    mCameraManager(*ODFrameListener::getSingleton().getCameraManager()),
    mTopLeftCornerX(0),
    mTopLeftCornerY(0),
    mWidth(0),
    mHeight(0),
    mMapX(mGameMap.getMapSizeX()),
    mMapY(mGameMap.getMapSizeY()),
    mElapsedTime(MIN_TIME_REFRESH_SECS),
    mMiniMapOgreTexture(Ogre::TextureManager::getSingletonPtr()->createManual(
            "miniMapOgreTexture",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::TEX_TYPE_2D,
            TEXTURE_SIZE, TEXTURE_SIZE, 0, Ogre::PF_R8G8B8,
            Ogre::TU_RENDERTARGET)),
    mCurCamPosX(-1),
    mCurCamPosY(-1),
    mMiniMapCam(RenderManager::getSingleton().getSceneManager()->createCamera("miniMapCam")),
    mMiniMapCamNode(RenderManager::getSingleton().getSceneManager()->createSceneNode("minimapCameraNode")),
    mCullingManager(new CullingManager(&mGameMap, CullingType::SHOW_MINIMAP)),
    mCameraTilesIntersections(std::vector<Ogre::Vector3>(4, Ogre::Vector3::ZERO))
{

    // We create a special camera that will look at the whole scene and render it in the minimap
    mMiniMapCam->setNearClipDistance(0.02f);
    mMiniMapCam->setFarClipDistance(300.0f);
    mMiniMapCam->setFOVy(ANGLE_CAM);

    Ogre::RenderTarget* rt = mMiniMapOgreTexture->getBuffer()->getRenderTarget();
    rt->addListener(this);
    rt->setAutoUpdated(false);
    mMiniMapCam->setAspectRatio(1.0);
    Ogre::Viewport* vp = rt->addViewport(mMiniMapCam);
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

    mTopLeftCornerX = mMiniMapWindow->getUnclippedOuterRect().get().getPosition().d_x;
    mTopLeftCornerY = mMiniMapWindow->getUnclippedOuterRect().get().getPosition().d_y;
    mWidth = mMiniMapWindow->getUnclippedOuterRect().get().getSize().d_width;
    mHeight = mMiniMapWindow->getUnclippedOuterRect().get().getSize().d_height;

    updateMinimapCamera();
    //mCullingManager->computeIntersectionPoints(mMiniMapCam, mCameraTilesIntersections);
    //mCullingManager->startTileCulling(mMiniMapCam, mCameraTilesIntersections);
}

MiniMapCamera::~MiniMapCamera()
{
    delete mCullingManager;

    mMiniMapWindow->setProperty("Image", "");
    Ogre::RenderTarget* rt = mMiniMapOgreTexture->getBuffer()->getRenderTarget();
    rt->removeAllListeners();
    Ogre::SceneManager* sceneManager = RenderManager::getSingleton().getSceneManager();
    sceneManager->destroyCamera(mMiniMapCam);
    Ogre::TextureManager::getSingletonPtr()->remove("miniMapOgreTexture");
    CEGUI::ImageManager::getSingletonPtr()->destroy("MiniMapImageset");
    CEGUI::System::getSingletonPtr()->getRenderer()->destroyTexture("miniMapTextureGui");
}

Ogre::Vector2 MiniMapCamera::camera_2dPositionFromClick(int xx, int yy)
{
    if(mCurCamPosX == -1)
        return Ogre::Vector2::ZERO;

    if(mCurCamPosY == -1)
        return Ogre::Vector2::ZERO;

    const Ogre::Quaternion& orientation = mCameraManager.getActiveCameraNode()->getOrientation();
    Ogre::Radian angle = orientation.getRoll();
    Ogre::Real cos = Ogre::Math::Cos(angle);
    Ogre::Real sin = Ogre::Math::Sin(angle);

    // Compute tile clicked
    Ogre::Real diffX = ((xx - mTopLeftCornerX) / mWidth - 0.5) * NB_TILES_DISPLAYED_IN_MINIMAP;
    Ogre::Real diffY = ((mTopLeftCornerY - yy) / mHeight + 0.5) * NB_TILES_DISPLAYED_IN_MINIMAP;

    Ogre::Vector2 pos(diffX * cos - diffY * sin + mCurCamPosX, diffX * sin + diffY * cos + mCurCamPosY);
    OD_LOG_INF("Clicked minimap pos=" + Helper::toString(pos.x) + ", " + Helper::toString(pos.y));
    return pos;
}

void MiniMapCamera::update(Ogre::Real timeSinceLastFrame, const std::vector<Ogre::Vector3>& cornerTiles)
{
    mElapsedTime += timeSinceLastFrame;
    if(mElapsedTime < MIN_TIME_REFRESH_SECS)
        return;

    mElapsedTime = 0;
    updateMinimapCamera();
    //mCullingManager->computeIntersectionPoints(mMiniMapCam, mCameraTilesIntersections);
    //mCullingManager->update(mMiniMapCam, mCameraTilesIntersections);

    Ogre::RenderTarget* rt = mMiniMapOgreTexture->getBuffer()->getRenderTarget();
    rt->update();
}

void MiniMapCamera::updateMinimapCamera()
{
    const Ogre::Vector3& camPos = mCameraManager.getActiveCameraNode()->getPosition();
    mCurCamPosX = Helper::round(camPos.x);
    mCurCamPosY = Helper::round(camPos.y);

    const Ogre::Quaternion& orientation = mCameraManager.getActiveCameraNode()->getOrientation();
    mMiniMapCamNode->setPosition(mCurCamPosX, mCurCamPosY, CAM_HEIGHT);
    mMiniMapCamNode->lookAt(Ogre::Vector3(mCurCamPosX, mCurCamPosY, 0.0),
                            Ogre::Node::TransformSpace::TS_LOCAL);
    mMiniMapCamNode->roll(orientation.getRoll());
}

void MiniMapCamera::preRenderTargetUpdate(const Ogre::RenderTargetEvent& rte)
{
    RenderManager::getSingleton().rrMinimapRendering(false);
}

void MiniMapCamera::postRenderTargetUpdate(const Ogre::RenderTargetEvent& rte)
{
    RenderManager::getSingleton().rrMinimapRendering(true);
}
