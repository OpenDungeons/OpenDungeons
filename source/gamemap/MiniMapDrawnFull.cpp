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

#include "gamemap/MiniMapDrawnFull.h"

#include "entities/GameEntityType.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "render/ODFrameListener.h"

#include <OgrePrerequisites.h>
#include <OgreSceneNode.h>
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

class MiniMapDrawnFullTileStateListener : public TileStateListener
{
public:
    MiniMapDrawnFullTileStateListener(MiniMapDrawnFull& minimap,
            uint32_t minimapXMin, uint32_t minimapXMax, uint32_t minimapYMin,
            uint32_t minimapYMax, uint32_t tileXMin, uint32_t tileXMax,
            uint32_t tileYMin, uint32_t tileYMax) :
        mMinimapXMin(minimapXMin),
        mMinimapXMax(minimapXMax),
        mMinimapYMin(minimapYMin),
        mMinimapYMax(minimapYMax),
        mTileXMin(tileXMin),
        mTileXMax(tileXMax),
        mTileYMin(tileYMin),
        mTileYMax(tileYMax),
        mMinimap(minimap)
    {}

    virtual ~MiniMapDrawnFullTileStateListener()
    {}

    virtual void tileStateChanged(Tile& tile) override
    {
        fireTileStateChanged();
    }

    void fireTileStateChanged()
    {
        mMinimap.updateTileState(mMinimapXMin, mMinimapXMax, mMinimapYMin,
            mMinimapYMax, mTileXMin, mTileXMax, mTileYMin, mTileYMax);
    }

    const uint32_t mMinimapXMin;
    const uint32_t mMinimapXMax;
    const uint32_t mMinimapYMin;
    const uint32_t mMinimapYMax;
    const uint32_t mTileXMin;
    const uint32_t mTileXMax;
    const uint32_t mTileYMin;
    const uint32_t mTileYMax;

private:
    MiniMapDrawnFull& mMinimap;
};

//! \brief This enum represents the possible values a pixel can have in the minimap.
//! If a pixel corresponds to several game tiles, the highest value will be used
//! to display the pixel
enum class MiniMapDrawnFullPixel
{
    dirtFull,
    dirtGround,
    rockFull,
    rockGround,
    claimedFull,
    claimedGround,
    lava,
    water,
    goldFull,
    goldGround,
    gemFull,
    gemGround,
    pickupEntity,
    alliedCreature,
    enemyCreature
};

namespace
{
MiniMapDrawnFullPixel getPixelValueFromTile(Seat& playerSeat, Tile& tile)
{
    MiniMapDrawnFullPixel value = MiniMapDrawnFullPixel::dirtFull;
    const std::vector<GameEntity*>& entities = tile.getEntitiesInTile();
    for(GameEntity* entity : entities)
    {
        if(entity->getObjectType() == GameEntityType::creature)
        {
            if(!entity->getSeat()->isAlliedSeat(&playerSeat))
            {
                if(value < MiniMapDrawnFullPixel::enemyCreature)
                    value = MiniMapDrawnFullPixel::enemyCreature;
            }
            else
            {
                if(value < MiniMapDrawnFullPixel::alliedCreature)
                    value = MiniMapDrawnFullPixel::alliedCreature;
            }
        }
        else if(entity->tryPickup(&playerSeat))
        {
            if(value < MiniMapDrawnFullPixel::pickupEntity)
                value = MiniMapDrawnFullPixel::pickupEntity;
        }
    }

    // If something interesting is on the tile, we return the computed value
    if(value != MiniMapDrawnFullPixel::dirtFull)
        return value;

    // Otherwise, we compute a value according to its type
    switch(tile.getTileVisual())
    {
        case TileVisual::lavaGround:
            value = MiniMapDrawnFullPixel::lava;
            break;
        case TileVisual::waterGround:
            value = MiniMapDrawnFullPixel::water;
            break;
        case TileVisual::goldFull:
            value = MiniMapDrawnFullPixel::goldFull;
            break;
        case TileVisual::goldGround:
            value = MiniMapDrawnFullPixel::goldGround;
            break;
        case TileVisual::gemFull:
            value = MiniMapDrawnFullPixel::gemFull;
            break;
        case TileVisual::gemGround:
            value = MiniMapDrawnFullPixel::gemGround;
            break;
        case TileVisual::rockFull:
            value = MiniMapDrawnFullPixel::rockFull;
            break;
        case TileVisual::rockGround:
            value = MiniMapDrawnFullPixel::rockGround;
            break;
        case TileVisual::claimedGround:
            value = MiniMapDrawnFullPixel::claimedGround;
            break;
        case TileVisual::claimedFull:
            value = MiniMapDrawnFullPixel::claimedFull;
            break;
        case TileVisual::dirtGround:
            value = MiniMapDrawnFullPixel::dirtGround;
            break;
        case TileVisual::dirtFull:
            value = MiniMapDrawnFullPixel::dirtFull;
            break;
        default:
            break;
    }

    return value;
}

void colourFromPixelValue(MiniMapDrawnFullPixel pixelValue, Seat* seatIfClaimed,
        Ogre::HardwarePixelBufferSharedPtr pixelBuffer, Ogre::PixelBox pixelBox,
        uint32_t minimapWidth, uint32_t minimapHeight, uint32_t minimapXMin,
        uint32_t minimapXMax, uint32_t minimapYMin, uint32_t minimapYMax)
{
    Ogre::uint8 RR = 0x00;
    Ogre::uint8 GG = 0x00;
    Ogre::uint8 BB = 0x00;
    switch(pixelValue)
    {
        case MiniMapDrawnFullPixel::enemyCreature:
        {
            RR = 0xFF;
            GG = 0x00;
            BB = 0x00;
            break;
        }
        case MiniMapDrawnFullPixel::claimedFull:
        {
            if(seatIfClaimed == nullptr)
            {
                RR = 0x86;
                GG = 0x50;
                BB = 0x28;
            }
            else
            {
                const Ogre::ColourValue& color = seatIfClaimed->getColorValue();
                RR = color.r * 255.0;
                GG = color.g * 255.0;
                BB = color.b * 255.0;
            }
            break;
        }
        case MiniMapDrawnFullPixel::claimedGround:
        {
            if(seatIfClaimed == nullptr)
            {
                RR = 0x5C;
                GG = 0x37;
                BB = 0x1B;
            }
            else
            {
                const Ogre::ColourValue& color = seatIfClaimed->getColorValue();
                RR = color.r * 200.0;
                GG = color.g * 200.0;
                BB = color.b * 200.0;
            }
            break;
        }
        case MiniMapDrawnFullPixel::goldFull:
        {
            RR = 0xB5;
            GG = 0xB3;
            BB = 0x2F;
            break;
        }
        case MiniMapDrawnFullPixel::goldGround:
        {
            RR = 0x3B;
            GG = 0x1D;
            BB = 0x08;
            break;
        }
        case MiniMapDrawnFullPixel::water:
        {
            RR = 0x21;
            GG = 0x36;
            BB = 0x7A;
            break;
        }
        case MiniMapDrawnFullPixel::lava:
        {
            RR = 0xB2;
            GG = 0x22;
            BB = 0x22;
            break;
        }
        case MiniMapDrawnFullPixel::dirtGround:
        {
            RR = 0x3B;
            GG = 0x1D;
            BB = 0x08;
            break;
        }
        case MiniMapDrawnFullPixel::dirtFull:
        {
            RR = 0x5B;
            GG = 0x2D;
            BB = 0x0C;
            break;
        }
        case MiniMapDrawnFullPixel::rockGround:
        {
            RR = 0x30;
            GG = 0x30;
            BB = 0x30;
            break;
        }
        case MiniMapDrawnFullPixel::rockFull:
        {
            RR = 0x41;
            GG = 0x41;
            BB = 0x41;
            break;
        }
        default:
        {
            RR = 0x00;
            GG = 0x00;
            BB = 0xFF;
            break;
        }
        case MiniMapDrawnFullPixel::pickupEntity:
        {
            RR = 0xDD;
            GG = 0xDD;
            BB = 0x12;
            break;
        }
    }

    pixelBuffer->lock(pixelBox, Ogre::HardwareBuffer::HBL_NORMAL);
    for(uint32_t x = minimapXMin; x < minimapXMax; ++x)
    {
        for(uint32_t y = minimapYMin; y < minimapYMax; ++y)
        {
            Ogre::uint8* pDest = static_cast<Ogre::uint8*>(pixelBuffer->getCurrentLock().data) - 1;
            pDest += (minimapWidth * (minimapHeight - y - 1) * 4);
            pDest += (x * 4);

            pDest++; //A, unused, shouldn't be here
            // this is the order of colors I empirically found out to be working :)
            *pDest++ = BB;  //B
            *pDest++ = GG;  //G
            *pDest++ = RR;  //R
        }
    }
    pixelBuffer->unlock();
}
}

MiniMapDrawnFull::MiniMapDrawnFull(CEGUI::Window* miniMapWindow) :
    mMiniMapWindow(miniMapWindow),
    mGameMap(*ODFrameListener::getSingleton().getClientGameMap()),
    mCameraManager(*ODFrameListener::getSingleton().getCameraManager()),
    mTopLeftCornerX(0),
    mTopLeftCornerY(0),
    mWidth(static_cast<unsigned int>(mMiniMapWindow->getPixelSize().d_width)),
    mHeight(static_cast<unsigned int>(mMiniMapWindow->getPixelSize().d_height)),
    mPixelBox(mWidth, mHeight, 1, Ogre::PF_R8G8B8),
    mMiniMapOgreTexture(Ogre::TextureManager::getSingletonPtr()->createManual(
            "miniMapOgreTexture",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::TEX_TYPE_2D,
            mWidth, mHeight, 0, Ogre::PF_R8G8B8,
            Ogre::TU_DYNAMIC_WRITE_ONLY)),
    mPixelBuffer(mMiniMapOgreTexture->getBuffer())
{
    uint32_t tileXMax = mGameMap.getMapSizeX();
    uint32_t tileYMax = mGameMap.getMapSizeY();
    uint32_t tileX = 0;
    uint32_t tileY = 0;
    uint32_t mapX = 0;
    uint32_t mapY = 0;

    Ogre::Real gainX = static_cast<Ogre::Real>(mWidth) / static_cast<Ogre::Real>(tileXMax);
    Ogre::Real gainY = static_cast<Ogre::Real>(mHeight) / static_cast<Ogre::Real>(tileYMax);

    while((mapX < mWidth) && (mapY < mHeight) && (tileX < tileXMax) && (tileY < tileYMax))
    {
        uint32_t tileXNext = static_cast<uint32_t>(round(static_cast<Ogre::Real>(mapX + 1) / gainX));
        uint32_t tileYNext = static_cast<uint32_t>(round(static_cast<Ogre::Real>(mapY + 1) / gainY));
        uint32_t mapXNext = static_cast<uint32_t>(round(static_cast<Ogre::Real>(tileX + 1) * gainX));
        uint32_t mapYNext = static_cast<uint32_t>(round(static_cast<Ogre::Real>(tileY + 1) * gainY));

        if(tileXNext == tileX)
            ++tileXNext;
        if(tileYNext == tileY)
            ++tileYNext;
        if(mapXNext == mapX)
            ++mapXNext;
        if(mapYNext == mapY)
            ++mapYNext;

        MiniMapDrawnFullTileStateListener* listener = new MiniMapDrawnFullTileStateListener(*this,
            mapX, mapXNext, mapY, mapYNext, tileX, tileXNext, tileY, tileYNext);

        mTileStateListeners.push_back(listener);

        mapX = mapXNext;
        tileX = tileXNext;
        if(tileXNext >= tileXMax)
        {
            mapY = mapYNext;
            tileY = tileYNext;
            mapX = 0;
            tileX = 0;
        }
    }

    // It  start, we fire the tile state changed event to make sure every pixel is
    // correctly initialized. We also set the listeners on the tiles
    for(MiniMapDrawnFullTileStateListener* listener : mTileStateListeners)
    {
        for(uint32_t xxx = listener->mTileXMin; xxx < listener->mTileXMax; ++xxx)
        {
            for(uint32_t yyy = listener->mTileYMin; yyy < listener->mTileYMax; ++yyy)
            {
                Tile* tile = mGameMap.getTile(xxx, yyy);
                if(tile == nullptr)
                    continue;

                tile->addTileStateListener(*listener);
            }
        }

        listener->fireTileStateChanged();
    }

    CEGUI::Texture& miniMapTextureGui = static_cast<CEGUI::OgreRenderer*>(CEGUI::System::getSingletonPtr()
                                            ->getRenderer())->createTexture("miniMapTextureGui", mMiniMapOgreTexture);

    CEGUI::BasicImage& imageset = dynamic_cast<CEGUI::BasicImage&>(CEGUI::ImageManager::getSingletonPtr()->create("BasicImage", "MiniMapImageset"));
    imageset.setArea(CEGUI::Rectf(CEGUI::Vector2f(0.0, 0.0),
                                      CEGUI::Size<float>(
                                          static_cast<float>(mWidth), static_cast<float>(mHeight)
                                      )
                                  ));

    // Link the image to the minimap
    imageset.setTexture(&miniMapTextureGui);
    mMiniMapWindow->setProperty("Image", CEGUI::PropertyHelper<CEGUI::Image*>::toString(&imageset));

    mMiniMapOgreTexture->load();

    mTopLeftCornerX = mMiniMapWindow->getUnclippedOuterRect().get().getPosition().d_x;
    mTopLeftCornerY = mMiniMapWindow->getUnclippedOuterRect().get().getPosition().d_y;
}

MiniMapDrawnFull::~MiniMapDrawnFull()
{
    for(MiniMapDrawnFullTileStateListener* listener : mTileStateListeners)
    {
        for(uint32_t xxx = listener->mTileXMin; xxx < listener->mTileXMax; ++xxx)
        {
            for(uint32_t yyy = listener->mTileYMin; yyy < listener->mTileYMax; ++yyy)
            {
                Tile* tile = mGameMap.getTile(xxx, yyy);
                if(tile == nullptr)
                    continue;

                tile->removeTileStateListener(*listener);
            }
        }
        delete listener;
    }
    mTileStateListeners.clear();

    mMiniMapWindow->setProperty("Image", "");
    Ogre::TextureManager::getSingletonPtr()->remove("miniMapOgreTexture");
    CEGUI::ImageManager::getSingletonPtr()->destroy("MiniMapImageset");
    CEGUI::System::getSingletonPtr()->getRenderer()->destroyTexture("miniMapTextureGui");
}

Ogre::Vector2 MiniMapDrawnFull::camera_2dPositionFromClick(int xx, int yy)
{
    Ogre::Vector2 v(0, 0);
    Ogre::Real gainX = static_cast<Ogre::Real>(mGameMap.getMapSizeX())
        / static_cast<Ogre::Real>(mWidth);
    Ogre::Real gainY = static_cast<Ogre::Real>(mGameMap.getMapSizeY())
        / static_cast<Ogre::Real>(mHeight);

    v.x = round(static_cast<Ogre::Real>(xx - mTopLeftCornerX) * gainX);
    v.y = round(static_cast<Ogre::Real>(mHeight - yy + mTopLeftCornerY) * gainY);

    return v;
}

void MiniMapDrawnFull::updateTileState(uint32_t minimapXMin, uint32_t minimapXMax,
        uint32_t minimapYMin, uint32_t minimapYMax, uint32_t tileXMin,
        uint32_t tileXMax, uint32_t tileYMin, uint32_t tileYMax)
{
    Seat& localPlayerSeat = *(mGameMap.getLocalPlayer()->getSeat());
    // We compute the tile representation
    MiniMapDrawnFullPixel curValue = MiniMapDrawnFullPixel::dirtFull;
    Seat* seatIfClaimed = nullptr;
    for(uint32_t xxx = tileXMin; xxx < tileXMax; ++xxx)
    {
        for(uint32_t yyy = tileYMin; yyy < tileYMax; ++yyy)
        {
            Tile* tile = mGameMap.getTile(xxx, yyy);
            if(tile == nullptr)
                continue;

            MiniMapDrawnFullPixel value = getPixelValueFromTile(localPlayerSeat,
                *tile);
            if(value > curValue)
            {
                curValue = value;
                switch(value)
                {
                    case MiniMapDrawnFullPixel::claimedGround:
                    case MiniMapDrawnFullPixel::claimedFull:
                        seatIfClaimed = tile->getSeat();
                        break;
                    default:
                        break;
                }
            }
        }
    }

    // We paint corresponding pixels
    colourFromPixelValue(curValue, seatIfClaimed, mPixelBuffer, mPixelBox,
        mWidth, mHeight, minimapXMin, minimapXMax, minimapYMin, minimapYMax);
}
