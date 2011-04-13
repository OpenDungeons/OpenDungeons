/*!
 * \file   MiniMap.cpp
 * \date   13 April 2011
 * \author StefanP.MUC
 * \brief  Contains everything that is related to the minimap
 */

#include <CEGUI.h>
#include <RendererModules/Ogre/CEGUIOgreRenderer.h>

#include "Globals.h"
#include "GameMap.h"
#include "ODApplication.h"
#include "RenderManager.h"
#include "Gui.h"

#include "MiniMap.h"

template<> MiniMap* Ogre::Singleton<MiniMap>::ms_Singleton = 0;

/*! Initializes the MiniMap
 *
 */
MiniMap::MiniMap() :
        miniMapCamera(0),
        miniMapOgreTexture(0),
        miniMapRenderer(0)
{
    /* TODO: separate some of this code in own functions to make it possible
     * to change cameras from outside (for example to recalculate it after a
     * new level was loaded)
     */
    miniMapOgreTexture = Ogre::TextureManager::getSingletonPtr()
            ->createManual(
                    "miniMapOgreTexture",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    Ogre::TEX_TYPE_2D,
                    512, 512, 0, Ogre::PF_R8G8B8,
                    Ogre::TU_RENDERTARGET);

    miniMapRenderer = miniMapOgreTexture->getBuffer()->getRenderTarget();

    miniMapCamera = RenderManager::getSingletonPtr()
            ->sceneManager->createCamera("miniMapCamera");
    Ogre::Camera* mCamera = ODApplication::getSingletonPtr()->getCamera();
    miniMapCamera->setNearClipDistance(mCamera->getNearClipDistance());
    miniMapCamera->setFarClipDistance(mCamera->getFarClipDistance());
    miniMapCamera->setAspectRatio(1);
    //TODO: autocalculate position and lookAt
    miniMapCamera->setPosition(mCamera->getRealPosition());
    miniMapCamera->lookAt(mCamera->getRealDirection());

    Ogre::Viewport* vp = miniMapRenderer->addViewport(miniMapCamera, 1);
    vp->setClearEveryFrame(true);
    vp->setOverlaysEnabled(false);
    vp->setBackgroundColour(Ogre::ColourValue::Black);

    CEGUI::Texture& miniMapTextureGui
            = static_cast<CEGUI::OgreRenderer*>(CEGUI::System::getSingletonPtr()
                    ->getRenderer())->createTexture(miniMapOgreTexture);

    CEGUI::Imageset& imageset = CEGUI::ImagesetManager::getSingletonPtr()
            ->create("MiniMapImageset", miniMapTextureGui);
    imageset.defineImage("MiniMapImage",
            CEGUI::Point(0.0f, 0.0f),
            CEGUI::Size(miniMapTextureGui.getSize().d_width,
                    miniMapTextureGui.getSize().d_height),
            CEGUI::Point(0.0f,0.0f));

    //retrieve the CEGUI StaticImage(window) used to render the minimap
    CEGUI::WindowManager::getSingleton().getWindow(Gui::MINIMAP)->setProperty(
            "Image", CEGUI::PropertyHelper::imageToString(
                    &imageset.getImage("MiniMapImage")));
}

/*! \brief Returns a reference to the singleton object
 *
 */
MiniMap& MiniMap::getSingleton()
{
    assert(ms_Singleton);
    return(*ms_Singleton);
}

/*! \brief Returns a pointer to the singleton object
 *
 */
MiniMap* MiniMap::getSingletonPtr()
{
    return ms_Singleton;
}
