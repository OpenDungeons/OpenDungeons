/*!
 * \file   MiniMap.h
 * \date   13 April 2011
 * \author StefanP.MUC
 * \brief  header for the minimap
 */

#ifndef MINIMAP_H_
#define MINIMAP_H_

#include <OgreTexture.h>
#include <OgreSingleton.h>
#include <RendererModules/Ogre/CEGUIOgreRenderer.h>
#include "GameMap.h"
#include "Tile.h"
#include "Creature.h"

struct color{
    Ogre::uint8 RR ; Ogre::uint8 BB ; Ogre::uint8 GG ;
    color(){};
    color(Ogre::uint8 rr , Ogre::uint8 bb , Ogre::uint8 gg){
	RR=rr;
	BB=bb;
	GG=gg;

    }

};




class MiniMap : public Ogre::Singleton<MiniMap>
{
    public:
        MiniMap(GameMap*);
        ~MiniMap();
	void draw();
	void swap();
	Ogre::uint getWidth(){return width;}
	Ogre::uint getHeight(){return height; }
	std::vector<Creature*>::iterator updatedCreatureIndex;
	void setCamera_2dPosition( Ogre::Vector3 vv);
	Ogre::Vector2 camera_2dPositionFromClick( int xx, int yy);
        
    private:
        int allocateMiniMapMemory();
	inline void drawPixel(int xx ,int yy, Ogre::uint8 RR , Ogre::uint8 BB , Ogre::uint8 GG ){
	    for(int gg=0;gg<grainSize;gg++){
		for(int hh=0;hh<grainSize;hh++){
	    
		    tiles[xx+gg][yy+hh]=color(RR,GG,BB);

		}
	    }
	
	}
	inline void drawPixelToMemory(   Ogre::uint8*& pDest, unsigned char RR, unsigned char GG, unsigned char BB){
	     pDest++; //A, unused, shouldn't be here
	     // this is the order of colors I empirically found outto be working :)
            *pDest++ = BB;  //B
            *pDest++ = GG;  //G
            *pDest++ = RR;  //R


	}

	void drawPixelToMemory();
        MiniMap(const MiniMap&);

        // Private Data:

	int topLeftCornerY;
	int topLeftCornerX;
	int grainSize;
	Ogre::Vector2 camera_2dPosition;
	int miniMapSizeX;
	int miniMapSizeY;
	color** tiles;
	GameMap* gameMap;
        Ogre::TexturePtr miniMapOgreTexture;
        Ogre::uint width, height;
	Ogre::HardwarePixelBufferSharedPtr pixelBuffer;
	Ogre::PixelBox* pixelBox;
};

#endif /* MINIMAP_H_ */
