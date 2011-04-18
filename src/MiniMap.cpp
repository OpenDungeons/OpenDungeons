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
        miniMapOgreTexture(0),
        miniMapRenderer(0)
{
    /* TODO: separate some of this code in own functions to make it possible
     * to change cameras from outside (for example to recalculate it after a
     * new level was loaded)
     */
    miniMapOgreTexture = Ogre::TextureManager::getSingletonPtr()->createManual(
            "miniMapOgreTexture",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::TEX_TYPE_2D,
            200, 200, 1, Ogre::PF_R8G8B8,
            Ogre::TU_RENDERTARGET);

    Ogre::HardwarePixelBufferSharedPtr texturePixelBuffer
            = miniMapOgreTexture->getBuffer();
    texturePixelBuffer->lock(Ogre::HardwareBuffer::HBL_NORMAL); // for best performance use HBL_DISCARD!
    const Ogre::PixelBox& texturePixelBox = texturePixelBuffer->getCurrentLock();

    miniMapRenderer = miniMapOgreTexture->getBuffer()->getRenderTarget();

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

    CEGUI::Image bla = imageset.getImage("MiniMapImage");

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
