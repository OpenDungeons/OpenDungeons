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

ChildOverlay::ChildOverlay(const Ogre::String& fontName, Ogre::Real charHeight,
        const Ogre::ColourValue& color, const Ogre::String& materialName) :
    mOverlayContainer(nullptr),
    mOverlayText(nullptr),
    mTextWidth(0),
    mTextHeight(0),
    mForcedWidth(-1),
    mForcedHeight(-1),
    mCharHeight(charHeight),
    mTimeToDisplay(0),
    mFont(dynamic_cast<Ogre::Font*>(Ogre::FontManager::getSingleton().getByName(fontName).getPointer()))
{
    OD_ASSERT_TRUE_MSG(mFont != nullptr, "fontName=" + fontName);
    mFont->load();
}

void ChildOverlay::setMaterialName(const Ogre::String& materialName)
{
    if(mMaterialName == materialName)
        return;

    mMaterialName = materialName;
    mOverlayContainer->setMaterialName(mMaterialName);
}

void ChildOverlay::setCaption(const Ogre::String& caption)
{
    if (caption != mCaption)
    {
        mCaption = caption;
        mOverlayText->setCaption(mCaption);
        computeTextArea();
    }
}

void ChildOverlay::forceTextArea(Ogre::Real textWidth, Ogre::Real textHeight)
{
    mForcedWidth = textWidth;
    mForcedHeight = textHeight;
}

void ChildOverlay::computeTextArea()
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

void ChildOverlay::displayOverlay(Ogre::Real time)
{
    if((mTimeToDisplay == 0) && (time != 0))
    {
        mOverlayContainer->show();
    }
    else if((mTimeToDisplay != 0) && (time == 0))
    {
        mOverlayContainer->hide();
    }

    mTimeToDisplay = time;
}

Ogre::Real ChildOverlay::getWidth()
{
    if(mForcedWidth != -1)
        return mForcedWidth;

    return mTextWidth;
}

Ogre::Real ChildOverlay::getHeight()
{
    if(mForcedHeight != -1)
        return mForcedHeight;

    return mTextHeight;
}

void ChildOverlay::update(Ogre::Real timeSincelastFrame)
{
    if(mTimeToDisplay <= 0.0)
        return;

    if(mTimeToDisplay > timeSincelastFrame)
    {
        mTimeToDisplay -= timeSincelastFrame;
        return;
    }

    mTimeToDisplay = 0.0;
    mOverlayContainer->hide();
}

bool ChildOverlay::isDisplayed()
{
    return mTimeToDisplay != 0.0;
}

MovableTextOverlay::MovableTextOverlay(const Ogre::String& name, const Ogre::MovableObject* followedMov,
        Ogre::Camera* camera) :
    mName(name),
    mFollowedMov(followedMov),
    mOverlay(nullptr),
    mOnScreen(false),
    mCamera(camera)
{
    // create an overlay that we can use for later
    Ogre::OverlayManager& overlayManager = Ogre::OverlayManager::getSingleton();
    mOverlay = overlayManager.create(name + "_Ov");
    mOverlay->show();
}

MovableTextOverlay::~MovableTextOverlay()
{
    mOverlay->hide();
    Ogre::OverlayManager& overlayManager = Ogre::OverlayManager::getSingleton();
    for(ChildOverlay& childOverlay : mChildOverlays)
    {
        childOverlay.mOverlayContainer->removeChild(childOverlay.mOverlayText->getName());
        mOverlay->remove2D(childOverlay.mOverlayContainer);
        overlayManager.destroyOverlayElement(childOverlay.mOverlayText);
        overlayManager.destroyOverlayElement(childOverlay.mOverlayContainer);
    }
    overlayManager.destroy(mOverlay);
}

uint32_t MovableTextOverlay::createChildOverlay(const Ogre::String& fontName, Ogre::Real charHeight,
    const Ogre::ColourValue& color, const Ogre::String& materialName)
{
    uint32_t id = mChildOverlays.size();
    Ogre::OverlayManager& overlayManager = Ogre::OverlayManager::getSingleton();
    ChildOverlay childOverlay(fontName, charHeight, color, materialName);
    childOverlay.mOverlayContainer = static_cast<Ogre::OverlayContainer*>(overlayManager.createOverlayElement(
        "Panel", mName + Helper::toString(id) + "_OvC"));
    childOverlay.mOverlayContainer->setDimensions(0.0, 0.0);

    mOverlay->add2D(childOverlay.mOverlayContainer);

    childOverlay.mOverlayText = overlayManager.createOverlayElement("TextArea", mName + Helper::toString(id) + "_OvTxt");
    childOverlay.mOverlayContainer->addChild(childOverlay.mOverlayText);

    childOverlay.mOverlayText->setMetricsMode(Ogre::GMM_RELATIVE);
    childOverlay.mOverlayText->setDimensions(1.0, 1.0);
    childOverlay.mOverlayText->setMetricsMode(Ogre::GMM_PIXELS);
    childOverlay.mOverlayText->setPosition(0, 0);

    childOverlay.setMaterialName(materialName);

    childOverlay.mOverlayText->setColour(color);

    childOverlay.mOverlayText->setParameter("font_name", fontName);
    childOverlay.mOverlayText->setParameter("char_height", Ogre::StringConverter::toString(charHeight));
    childOverlay.mOverlayText->setParameter("horz_align", "center");
    childOverlay.mOverlayText->setParameter("vert_align", "top");

    mChildOverlays.push_back(childOverlay);

    return id;
}

void MovableTextOverlay::setCaption(uint32_t childOverlayId, const Ogre::String& caption)
{
    if(childOverlayId >= mChildOverlays.size())
    {
        OD_ASSERT_TRUE_MSG(false, "childOverlayId=" + Helper::toString(childOverlayId));
        return;
    }

    ChildOverlay& childOverlay = mChildOverlays[childOverlayId];
    childOverlay.setCaption(caption);
}

void MovableTextOverlay::forceTextArea(uint32_t childOverlayId, Ogre::Real textWidth, Ogre::Real textHeight)
{
    if(childOverlayId >= mChildOverlays.size())
    {
        OD_ASSERT_TRUE_MSG(false, "childOverlayId=" + Helper::toString(childOverlayId));
        return;
    }

    ChildOverlay& childOverlay = mChildOverlays[childOverlayId];
    childOverlay.forceTextArea(textWidth, textHeight);
}

void MovableTextOverlay::setMaterialName(uint32_t childOverlayId, const Ogre::String& materialName)
{
    if(childOverlayId >= mChildOverlays.size())
    {
        OD_ASSERT_TRUE_MSG(false, "childOverlayId=" + Helper::toString(childOverlayId));
        return;
    }

    ChildOverlay& childOverlay = mChildOverlays[childOverlayId];
    childOverlay.setMaterialName(materialName);
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

void MovableTextOverlay::displayOverlay(uint32_t childOverlayId, Ogre::Real time)
{
    if(childOverlayId >= mChildOverlays.size())
    {
        OD_ASSERT_TRUE_MSG(false, "childOverlayId=" + Helper::toString(childOverlayId));
        return;
    }

    ChildOverlay& childOverlay = mChildOverlays[childOverlayId];
    childOverlay.displayOverlay(time);
}

void MovableTextOverlay::update(Ogre::Real timeSincelastFrame)
{
    bool displayed = false;
    for(ChildOverlay& childOverlay : mChildOverlays)
    {
        childOverlay.update(timeSincelastFrame);

        if(!childOverlay.isDisplayed())
            continue;

        displayed = true;
    }

    if(!displayed)
        return;

    Ogre::Vector2 screenPosition;
    mOnScreen = computeOverlayPositionHead(screenPosition);
    if(!mOnScreen)
        return;

    for(ChildOverlay& childOverlay : mChildOverlays)
    {
        if(!childOverlay.isDisplayed())
            continue;

        Ogre::Real relTextWidth = childOverlay.getWidth();
        Ogre::Real relTextHeight = childOverlay.getHeight();
        relTextWidth /= Ogre::OverlayManager::getSingleton().getViewportWidth();
        relTextHeight /= Ogre::OverlayManager::getSingleton().getViewportHeight();

        screenPosition.y -= relTextHeight;
        Ogre::Real xPos = screenPosition.x - (relTextWidth * 0.5);
        childOverlay.mOverlayContainer->setPosition(xPos, screenPosition.y);
        childOverlay.mOverlayContainer->setDimensions(relTextWidth, relTextHeight);
    }
}
