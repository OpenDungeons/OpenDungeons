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

#include <OgrePrerequisites.h>

namespace Ogre
{
class Font;
class Overlay;
class OverlayContainer;
class OverlayElement;
}

class ChildOverlay
{
    friend class MovableTextOverlay;
private:
    ChildOverlay(const Ogre::String& fontName, Ogre::Real charHeight,
        const Ogre::ColourValue& color, const Ogre::String& materialName);

    //! Computes the best size for the text to display
    void computeTextArea();

    void setCaption(const Ogre::String& caption);

    void forceTextArea(Ogre::Real textWidth, Ogre::Real textHeight);

    void displayOverlay(Ogre::Real time);

    void setMaterialName(const Ogre::String& materialName);

    void update(Ogre::Real timeSincelastFrame);

    bool isDisplayed();

    Ogre::Real getWidth();

    Ogre::Real getHeight();

    Ogre::OverlayContainer* mOverlayContainer;
    Ogre::OverlayElement* mOverlayText;

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

    //! Height a char has for the wanted font
    Ogre::Real mCharHeight;

    Ogre::Real mTimeToDisplay;

    //! Font used to display the text
    Ogre::Font* mFont;
};

class MovableTextOverlay
{
public:
    MovableTextOverlay(const Ogre::String& name, const Ogre::MovableObject *followedMov,
        Ogre::Camera* camera);

    virtual ~MovableTextOverlay();

    //! Creates a new ChildOverlay associated with this Text Overlay and returns its ID
    uint32_t createChildOverlay(const Ogre::String& fontName, Ogre::Real charHeight,
        const Ogre::ColourValue& color, const Ogre::String& materialName);

    //! \brief Set if the overlays should be displayed or not
    void setVisible(bool visible);

    //! Set the text displayed
    void setCaption(uint32_t childOverlayId, const Ogre::String& caption);

    //! Forces the text area size
    void forceTextArea(uint32_t childOverlayId, Ogre::Real textWidth, Ogre::Real textHeight);

    void setMaterialName(uint32_t childOverlayId, const Ogre::String& materialName);

    //! Displays the overlay during time seconds. If time < 0, the overlay will be always displayed
    void displayOverlay(uint32_t childOverlayId, Ogre::Real time);
    void update(Ogre::Real timeSincelastFrame);

private:
    //! Computes the position of the head of the followed entity in the screen coordinates. Returns true if
    //! the entity is on screen and position contains the position where the text should be displayed and false otherwise
    bool computeOverlayPositionHead(Ogre::Vector2& position);

    const Ogre::String mName;
    const Ogre::MovableObject* mFollowedMov;

    Ogre::Overlay* mOverlay;

    //! true if the upper vertices projections of the MovableObject are on screen
    bool mOnScreen;

    //! The camera used to display the scene
    const Ogre::Camera* mCamera;

    std::vector<ChildOverlay> mChildOverlays;
};
#endif // MOVABLETEXTOVERLAY_H
