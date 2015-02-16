/*!
 * \file   MiniMap.h
 * \date   13 April 2011
 * \author StefanP.MUC
 * \brief  header for the minimap
 *
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

#ifndef MINIMAP_H_
#define MINIMAP_H_

#include "render/Gui.h"

#include <OgreTexture.h>
#include <OgrePixelFormat.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreVector2.h>

#include <vector>

struct Color
{
public:
    Ogre::uint8 RR;
    Ogre::uint8 GG;
    Ogre::uint8 BB;

    Color():
        RR(0),
        GG(0),
        BB(0)
    {}

    Color(Ogre::uint8 rr, Ogre::uint8 gg, Ogre::uint8 bb):
        RR(rr),
        GG(gg),
        BB(bb)
    {}
};

//! \brief The class handling the minimap seen top-right of the in-game screen
//! FIXME: The pixel are displayed without taking in account the camera current roll value.
class MiniMap
{
public:
    MiniMap();
    ~MiniMap();

    void draw(const GameMap& gameMap);
    void swap();

    Ogre::uint getWidth() const
    { return mWidth; }

    Ogre::uint getHeight() const
    { return mHeight; }

    void updateCameraInfo(const Ogre::Vector3& vv, const double& rotation);

    Ogre::Vector2 camera_2dPositionFromClick(int xx, int yy);

    void attachMiniMap(Gui::guiSheet sheet);

private:
    Ogre::uint mWidth;
    Ogre::uint mHeight;

    int mTopLeftCornerX;
    int mTopLeftCornerY;
    int mGrainSize;

    Ogre::Vector2 mCamera_2dPosition;
    double mCosRotation, mSinRotation;

    typedef std::vector<Color> TileColorRow_t;
    typedef std::vector<TileColorRow_t> TileColorArray_t;
    //!brief Vector containing colours to be drawn.
    //NOTE: The tiles are laid out Y,X in the vector to iterate in the right order when drawing.
    TileColorArray_t mTiles;

    Ogre::TexturePtr mMiniMapOgreTexture;
    Ogre::HardwarePixelBufferSharedPtr mPixelBuffer;
    Ogre::PixelBox* mPixelBox;

    Gui::guiSheet mSheetUsed;

    inline void drawPixel(int xx, int yy, Ogre::uint8 RR, Ogre::uint8 GG, Ogre::uint8 BB)
    {
        for(int gg = 0; gg < mGrainSize; ++gg)
        {
            for(int hh = 0; hh < mGrainSize; ++hh)
            {
                mTiles[yy + hh][xx + gg] = Color(RR, GG, BB);
            }
        }

    }

    inline void drawPixelToMemory(Ogre::uint8*& pDest, unsigned char RR, unsigned char GG, unsigned char BB)
    {
        pDest++; //A, unused, shouldn't be here
        // this is the order of colors I empirically found outto be working :)
        *pDest++ = BB;  //B
        *pDest++ = GG;  //G
        *pDest++ = RR;  //R
    }
};

#endif // MINIMAP_H_
