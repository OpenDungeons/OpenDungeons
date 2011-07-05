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
        width(120),
        height(88)
{
    /* TODO: separate some of this code in own functions to make it possible
     * to change cameras from outside (for example to recalculate it after a
     * new level was loaded)
     */

    miniMapOgreTexture = Ogre::TextureManager::getSingletonPtr()->createManual(
            "miniMapOgreTexture",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::TEX_TYPE_2D,
            width, height, 0, Ogre::PF_R8G8B8,
            Ogre::TU_DYNAMIC_WRITE_ONLY);

    //Ogre::Image::Box pixelBox(0, 0, width, height);
    Ogre::PixelBox pixelBox(width, height, 0, Ogre::PF_R8G8B8);
    Ogre::HardwarePixelBufferSharedPtr pixelBuffer = miniMapOgreTexture->getBuffer();
    pixelBuffer->lock(pixelBox, Ogre::HardwareBuffer::HBL_NORMAL);

    Ogre::uint8* pDest = static_cast<Ogre::uint8*>(
            pixelBuffer->getCurrentLock().data) - 1;

    for(size_t i = 0; i < width; ++i)
    {
        for(size_t j = 0; j < height; ++j)
        {
            /*FIXME: even if we use a THREE byte pixel format (PF_R8G8B8),
             * for some reason it only works if we have FOUR increments
             * (the empty one is the unused alpha channel)
             * this is not how it is intended/expected
             */
            ++pDest; //A, unused, shouldn't be here
            *(++pDest) = 255;  //R
            *(++pDest) = 255;  //G
            *(++pDest) = 255;  //B
        }
    }
    pixelBuffer->unlock();
    miniMapOgreTexture->load();

    CEGUI::Texture& miniMapTextureGui
            = static_cast<CEGUI::OgreRenderer*>(CEGUI::System::getSingletonPtr()
                    ->getRenderer())->createTexture(miniMapOgreTexture);

    CEGUI::Imageset& imageset = CEGUI::ImagesetManager::getSingletonPtr()
            ->create("MiniMapImageset", miniMapTextureGui);
    imageset.defineImage("MiniMapImage",
            CEGUI::Point(0.0f, 0.0f),
            CEGUI::Size(width, height),
            CEGUI::Point(0.0f, 0.0f));

    CEGUI::WindowManager::getSingleton().getWindow(Gui::MINIMAP)->setProperty(
            "Image", CEGUI::PropertyHelper::imageToString(
                    &imageset.getImage("MiniMapImage")));
}

MiniMap::~MiniMap()
{
}
