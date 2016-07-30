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

#ifndef MINIMAPDRAWNFULL_H_
#define MINIMAPDRAWNFULL_H_

#include "gamemap/MiniMap.h"

#include <OgreHardwarePixelBuffer.h>
#include <OgrePixelFormat.h>
#include <OgreTexture.h>
#include <OgreVector2.h>
#include <OgreVector3.h>

#include <vector>

namespace CEGUI
{
    class Window;
}

class CameraManager;
class GameMap;
class MiniMapDrawnFullTileStateListener;
class Seat;
class Tile;

class MiniMapDrawnFull : public MiniMap
{
public:
    MiniMapDrawnFull(CEGUI::Window* miniMapWindow);
    ~MiniMapDrawnFull();

    Ogre::uint getWidth() const
    { return mWidth; }

    Ogre::uint getHeight() const
    { return mHeight; }

    void update(Ogre::Real timeSinceLastFrame) override
    {}

    void updateTileState(uint32_t minimapXMin, uint32_t xMinimapMax,
        uint32_t minimapYMin, uint32_t minimapYMax, uint32_t tileXMin,
        uint32_t tileXMax, uint32_t tileYMin, uint32_t tileYMax);

    Ogre::Vector2 camera_2dPositionFromClick(int xx, int yy) override;

private:
    CEGUI::Window* mMiniMapWindow;

    GameMap& mGameMap;
    CameraManager& mCameraManager;

    std::vector<MiniMapDrawnFullTileStateListener*> mTileStateListeners;

    int mTopLeftCornerX;
    int mTopLeftCornerY;
    Ogre::uint mWidth;
    Ogre::uint mHeight;

    Ogre::Vector2 mCamera_2dPosition;

    Ogre::PixelBox mPixelBox;
    Ogre::TexturePtr mMiniMapOgreTexture;
    Ogre::HardwarePixelBufferSharedPtr mPixelBuffer;
};

#endif // MINIMAPDRAWNFULL_H_
