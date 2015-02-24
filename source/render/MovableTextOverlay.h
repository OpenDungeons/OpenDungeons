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

#ifndef MOVABLETEXTOVERLAY_H
#define MOVABLETEXTOVERLAY_H

#include "Ogre.h"
#include "Overlay/OgreFont.h"
#include "Overlay/OgreFontManager.h"
#include "Overlay/OgreOverlayContainer.h"
#include "Overlay/OgreOverlayManager.h"

class MovableTextOverlay
{
public:
    MovableTextOverlay(const Ogre::String & name, const Ogre::MovableObject *followedMov,
        Ogre::Camera* camera, const Ogre::String& fontName, Ogre::Real charHeight,
        const Ogre::ColourValue& color, const Ogre::String& materialName);

    virtual ~MovableTextOverlay();

    //! Set the text displayed
    void setCaption(const Ogre::String& caption);

    //! Forces the text area size
    void forceTextArea(Ogre::Real textWidth, Ogre::Real textHeight);

    //! Changes the material displayed on the overlay
    void setMaterialName(const Ogre::String& materialName);

    inline const Ogre::String& getName() const
    { return mName; }

    inline const Ogre::String& getCaption() const
    { return mCaption; }

    inline const bool isOnScreen() const
    { return mOnScreen; }

    inline const bool isDisplayed() const
    { return mDisplayed; }

    inline const std::string& getMaterialName() const
    { return mMaterialName; }

    void setDisplay(bool display);
    void update(Ogre::Real timeSincelastFrame);

private:
    //! Computes the best size for the text to display
    void computeTextArea();

    //! Computes the position of the head of the followed entity in the screen coordinates. Returns true if
    //! the entity is on screen and position contains the position where the text should be displayed and false otherwise
    bool computeOverlayPositionHead(Ogre::Vector2& position);

    const Ogre::String mName;
    const Ogre::MovableObject* mFollowedMov;

    Ogre::Overlay* mOverlay;
    Ogre::OverlayContainer* mOverlayContainer;
    Ogre::OverlayElement* mOverlayText;

    //! true if the text should be displayed
    bool mDisplayed;

    //! the Material used in the overlay
    Ogre::String mMaterialName;

    //! the Text to display
    Ogre::String mCaption;

    //! Text width in pixels
    Ogre::Real mTextWidth;

    //! Text height in pixels
    Ogre::Real mTextHeight;

    //! Forced width in pixels
    Ogre::Real mForcedWidth;

    //! Forced height in pixels
    Ogre::Real mForcedHeight;

    //! true if the upper vertices projections of the MovableObject are on screen
    bool mOnScreen;

    //! The camera used to display the scene
    const Ogre::Camera* mCamera;

    //! Height a char has for the wanted font
    Ogre::Real mCharHeight;

    //! Font used to display the text
    Ogre::Font* mFont;
};
#endif // MOVABLETEXTOVERLAY_H
