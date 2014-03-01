/*!
 * \file   MiniMap.cpp
 * \date   13 April 2011
 * \author StefanP.MUC
 * \brief  Contains everything that is related to the minimap
 */
#include <cstdlib>

#include <OgrePrerequisites.h>

#include <CEGUI/WindowManager.h>
#include <CEGUI/Image.h>
#include <CEGUI/PropertyHelper.h>
#include <CEGUI/Texture.h>
#include <CEGUI/ImageManager.h>
#include <CEGUI/Size.h>
#include <CEGUI/BasicImage.h>

#include "ODApplication.h"
#include "RenderManager.h"
#include "Gui.h"
#include "Creature.h"
#include "MiniMap.h"
// #include <Magick++.h>


 // getDefaultGUIContext.getRootWindow // this way we can get root window


/*! Initializes the MiniMap
 *
 */
MiniMap::MiniMap(GameMap* gm) :
    topLeftCornerX(  Gui::getSingleton().sheets[Gui::inGameMenu]->getChild( Gui::MINIMAP )->getUnclippedOuterRect().get().getPosition().d_x),
    topLeftCornerY(  Gui::getSingleton().sheets[Gui::inGameMenu]->getChild( Gui::MINIMAP )->getUnclippedOuterRect().get().getPosition().d_y),
    grainSize(4),
    tiles(nullptr),
    gameMap(gm),
    width(  Gui::getSingleton().sheets[Gui::inGameMenu]->getChild( Gui::MINIMAP )->getPixelSize().d_width ),
    height(  Gui::getSingleton().sheets[Gui::inGameMenu]->getChild( Gui::MINIMAP )->getPixelSize().d_height),
    pixelBox(new Ogre::PixelBox (width, height, 1, Ogre::PF_R8G8B8))
{
    /* TODO: separate some of this code in own functions to make it possible
     * to change cameras from outside (for example to recalculate it after a
     * new level was loaded)
     */

    CEGUI::Window* inGameMenu = Gui::getSingleton().sheets[Gui::inGameMenu]  ;
    width  = (Ogre::uint)inGameMenu->getChild(Gui::MINIMAP)->getPixelSize().d_width;
    height = (Ogre::uint)inGameMenu->getChild(Gui::MINIMAP)->getPixelSize().d_height;
    topLeftCornerX = (int)inGameMenu->getChild(Gui::MINIMAP)->getUnclippedOuterRect().get().getPosition().d_x;
    topLeftCornerY = (int)inGameMenu->getChild(Gui::MINIMAP)->getUnclippedOuterRect().get().getPosition().d_y;

    gameMap->miniMap=this;
    allocateMiniMapMemory();
    // Image blank_image( Geometry(400, 300), Color(MaxRGB, MaxRGB, MaxRGB, 0));
    miniMapOgreTexture = Ogre::TextureManager::getSingletonPtr()->createManual(
            "miniMapOgreTexture",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::TEX_TYPE_2D,
            width, height, 0, Ogre::PF_R8G8B8,
            Ogre::TU_DYNAMIC_WRITE_ONLY);

    //Ogre::Image::Box pixelBox(0, 0, width, height);

    pixelBuffer = miniMapOgreTexture->getBuffer();



    //old draw code
    //old CEGUI code

    // Temporarily disabled until new CEGUI version works
    // Working game UI needed to test a possible fix for this!
    CEGUI::Texture& miniMapTextureGui
            = static_cast<CEGUI::OgreRenderer*>(CEGUI::System::getSingletonPtr()
						->getRenderer())->createTexture("miniMapTextureGui", miniMapOgreTexture);

    CEGUI::BasicImage& imageset = dynamic_cast<CEGUI::BasicImage&>(CEGUI::ImageManager::getSingletonPtr()->create("BasicImage","MiniMapImageset"));
    imageset.setArea(CEGUI::Rectf(CEGUI::Vector2f(0.0, 0.0), CEGUI::Size<float>((float)width, (float)height)));


    // imageset.defineImage("MiniMapImage",
    // 			 CEGUI::Vector2<float>(0.0f, 0.0f),
    // 			 CEGUI::Size<float>(width, height)
    // 			 );


    imageset.setTexture(&miniMapTextureGui);
    inGameMenu->getChild(Gui::MINIMAP)->setProperty("Image", CEGUI::PropertyHelper<CEGUI::Image*>::toString(&imageset));
    // CEGUI::WindowManager::getSingleton().getWindow(Gui::MINIMAP)->setProperty(
    //         "Image", CEGUI::PropertyHelper::imageToString(
    //                 &imageset.getImage("MiniMapImage")));

    miniMapOgreTexture->load();
    updatedCreatureIndex = gameMap->creatures.begin();
}

MiniMap::~MiniMap()
{
}

void MiniMap::setCamera_2dPosition( Ogre::Vector3 vv){

    camera_2dPosition = Ogre::Vector2(vv.x , vv.y);


}


Ogre::Vector2 MiniMap::camera_2dPositionFromClick(int xx, int yy)
{
    camera_2dPosition.x += (Ogre::Real)(((yy - topLeftCornerY) / double(height) - 0.5) * height / grainSize);
    camera_2dPosition.y += (Ogre::Real)(((xx - topLeftCornerX) / double(width) - 0.5) * width / grainSize);

    return camera_2dPosition;
}


void MiniMap::draw() {


    // Ogre::Vector3 halfCamera_2dPosition = camera_2dPosition /2;

    for (Ogre::uint ii = 0, mm = camera_2dPosition.x - width / (2 * grainSize); ii < width; ++mm, ii += grainSize)
	{
	    for (Ogre::uint jj = 0, nn = camera_2dPosition.y - height / (2 * grainSize); jj < height; ++nn, jj += grainSize)
		{
		    /*FIXME: even if we use a THREE byte pixel format (PF_R8G8B8),
		     * for some reason it only works if we have FOUR increments
		     * (the empty one is the unused alpha channel)
		     * this is not how it is intended/expected
		     */

		    if(gameMap->getTile(mm,nn)==NULL)
			drawPixel(ii,jj,0x00,0x00,0x00);
		    else{

			switch (gameMap->getTile(mm,nn)->getType()){


			case Tile::water:
			    drawPixel(ii,jj,0x7F,0xFF,0xD4);
			    break;

			case Tile::dirt:
			    drawPixel(ii,jj,0x8B,0x45,0x13);
			    break;

			case Tile::lava:
			    drawPixel(ii,jj,0xB2,0x22,0x22);
			    break;

			case Tile::rock:
			    drawPixel(ii,jj,0xA9,0xA9,0xA9);
			    break;

			case Tile::gold:
			    drawPixel(ii,jj,0xFF,0xD7,0xD0);
			    break;


			    // to be made more sophisticated :)
			case Tile::claimed:
			    drawPixel(ii,jj,0x94,0x00,0xD3);
			    break;

			case Tile::nullTileType:
			    drawPixel(ii,jj,0x00,0x00,0x00);
			    break;


			default:
			    drawPixel(ii,jj,0x00,0xFF,0x7F);
			    break;
			}

		    }
		}
	}

    // for(updatedCreatureIndex = gameMap->creatures.begin(); updatedCreatureIndex < gameMap->creatures.end(); ++updatedCreatureIndex)
    // {
    //     if((*updatedCreatureIndex)->getIsOnMap())
    //     {
    //         double  ii  = (*updatedCreatureIndex)->getPosition().x;
    //         double  jj  = (*updatedCreatureIndex)->getPosition().y;

    //         drawPixel(ii,jj,0x94,0x0,0x94);
    //     }
    // }
}

void MiniMap::swap()
{
    pixelBuffer->lock(*pixelBox, Ogre::HardwareBuffer::HBL_NORMAL     );

    Ogre::uint8* pDest;
    pDest = static_cast<Ogre::uint8*>( pixelBuffer->getCurrentLock().data) - 1;



    for (Ogre::uint ii = 0; ii < width  ; ++ii)
    	{
    	    for (Ogre::uint jj = 0; jj <  height; ++jj)
    		{


    		    drawPixelToMemory(pDest, tiles[ii][jj].RR, tiles[ii][jj].GG, tiles[ii][jj].BB );
    		}
    	}

    pixelBuffer->unlock();
}



int MiniMap::allocateMiniMapMemory()
{
    // miniMapSizeX = xSize;
    // miniMapSizeY=ySize;
    if (tiles == nullptr) {
        tiles = new color* [height];
        for (Ogre::uint jj = 0; jj < height; ++jj) {
            tiles[jj] = new color [width];
        }

        return 1;
    }
    else {
        std :: cerr << " failed to allocate minimap memory" << std :: endl;
        return 0;
    }
}
