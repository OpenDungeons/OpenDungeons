/*
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

#include "render/MovableTextOverlay.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <Overlay/OgreFontManager.h>
#include <OgrePrerequisites.h>

MovableTextOverlay::MovableTextOverlay(const Ogre::String & name, const Ogre::MovableObject *followedMov,
        Ogre::Camera* camera, const Ogre::String& fontName, Ogre::Real charHeight,
        const Ogre::ColourValue& color, const Ogre::String& materialName) :
    mName(name),
    mFollowedMov(followedMov),
    mOverlay(nullptr),
    mOverlayContainer(nullptr),
    mOverlayText(nullptr),
    mDisplayed(false),
    mCaption(""),
    mTextWidth(0),
    mTextHeight(0),
    mForcedWidth(-1),
    mForcedHeight(-1),
    mOnScreen(false),
    mCamera(camera),
    mCharHeight(charHeight),
    mFont(dynamic_cast<Ogre::Font*>(Ogre::FontManager::getSingleton().getByName(fontName).getPointer()))
{
    OD_ASSERT_TRUE_MSG(mFont != nullptr, "fontName=" + fontName);
    mFont->load();

    // create an overlay that we can use for later
    Ogre::OverlayManager& overlayManager = Ogre::OverlayManager::getSingleton();
    mOverlay = overlayManager.create(name + "_Ov");
    mOverlay->hide();
    mOverlayContainer = static_cast<Ogre::OverlayContainer*>(overlayManager.createOverlayElement(
        "Panel", name + "_OvC"));
    mOverlayContainer->setDimensions(0.0, 0.0);

    mOverlay->add2D(mOverlayContainer);

    mOverlayText = overlayManager.createOverlayElement("TextArea", name + "_OvTxt");
    mOverlayContainer->addChild(mOverlayText);

    mOverlayText->setMetricsMode(Ogre::GMM_RELATIVE);
    mOverlayText->setDimensions(1.0, 1.0);
    mOverlayText->setMetricsMode(Ogre::GMM_PIXELS);
    mOverlayText->setPosition(0, 0);

    setMaterialName(materialName);

    mOverlayText->setColour(color);

    mOverlayText->setParameter("font_name", fontName);
    mOverlayText->setParameter("char_height", Ogre::StringConverter::toString(charHeight));
    mOverlayText->setParameter("horz_align", "center");
    mOverlayText->setParameter("vert_align", "top");
}

MovableTextOverlay::~MovableTextOverlay()
{
    mOverlay->hide();
    Ogre::OverlayManager& overlayManager = Ogre::OverlayManager::getSingleton();
    mOverlayContainer->removeChild(mName + "_OvTxt");
    mOverlay->remove2D(mOverlayContainer);
    overlayManager.destroyOverlayElement(mOverlayText);
    overlayManager.destroyOverlayElement(mOverlayContainer);
    overlayManager.destroy(mOverlay);
}

void MovableTextOverlay::setCaption(const Ogre::String& caption)
{
    if (caption != mCaption)
    {
        mCaption = caption;
        mOverlayText->setCaption(mCaption);
        computeTextArea();
    }
}

void MovableTextOverlay::computeTextArea()
{
    mTextWidth = 0;

    for(auto c : mCaption)
    {
        if (c == 0x0020)
            mTextWidth += mFont->getGlyphAspectRatio(0x0030);
        else
            mTextWidth += mFont->getGlyphAspectRatio(c);
    }

    mTextWidth *= mCharHeight;
    mTextHeight = mCharHeight;
}

void MovableTextOverlay::forceTextArea(Ogre::Real textWidth, Ogre::Real textHeight)
{
    mForcedWidth = textWidth;
    mForcedHeight = textHeight;
}

void MovableTextOverlay::setMaterialName(const Ogre::String& materialName)
{
    if(mMaterialName == materialName)
        return;

    mMaterialName = materialName;
    mOverlayContainer->setMaterialName(mMaterialName);
}

bool MovableTextOverlay::computeOverlayPositionHead(Ogre::Vector2& position)
{
    if (!mFollowedMov->isInScene())
        return false;

    // the AABB of the target
    const Ogre::AxisAlignedBox& AABB = mFollowedMov->getWorldBoundingBox();
    const Ogre::Vector3 farLeftTop = AABB.getCorner(Ogre::AxisAlignedBox::NEAR_RIGHT_TOP);
    const Ogre::Vector3 nearRightTop = AABB.getCorner(Ogre::AxisAlignedBox::NEAR_LEFT_BOTTOM);

    const Ogre::Vector3 centerTop = (farLeftTop + nearRightTop) * 0.5;

    // Is the camera facing that point?
    Ogre::Plane cameraPlane(Ogre::Vector3(mCamera->getDerivedOrientation().zAxis()), mCamera->getDerivedPosition());
    if(cameraPlane.getSide(centerTop) != Ogre::Plane::NEGATIVE_SIDE)
        return false;

    // Transform 3D point to screen
    Ogre::Vector3 screenPosition = mCamera->getProjectionMatrix() * (mCamera->getViewMatrix() * centerTop);

    // We transform from coordinate space [-1, 1] to [0, 1]
    position.x = 0.5 + (screenPosition.x * 0.5);
    position.y = 0.5 - (screenPosition.y * 0.5);

    return true;
}

void MovableTextOverlay::setDisplay(bool display)
{
    if (mDisplayed == display)
        return;

    mDisplayed = display;
    if (mDisplayed)
        mOverlay->show();
    else
        mOverlay->hide();
}

void MovableTextOverlay::update(Ogre::Real timeSincelastFrame)
{
    if(!mDisplayed)
        return;

    Ogre::Vector2 screenPosition;
    mOnScreen = computeOverlayPositionHead(screenPosition);
    if(!mOnScreen)
        return;

    Ogre::Real relTextWidth = mTextWidth;
    if(mForcedWidth != -1)
        relTextWidth = mForcedWidth;
    relTextWidth /= Ogre::OverlayManager::getSingleton().getViewportWidth();

    Ogre::Real relTextHeight = mTextHeight;
    if(mForcedHeight != -1)
        relTextHeight = mForcedHeight;
    relTextHeight /= Ogre::OverlayManager::getSingleton().getViewportHeight();

    screenPosition.x -= relTextWidth * 0.5;
    screenPosition.y -= relTextHeight;

    mOverlayContainer->setPosition(screenPosition.x, screenPosition.y);
    mOverlayContainer->setDimensions(relTextWidth, relTextHeight);
}
