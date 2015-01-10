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

#ifndef TEXTRENDERER_H
#define TEXTRENDERER_H

#include <Overlay/OgreOverlayManager.h>
#include <Overlay/OgreOverlay.h>
#include <Overlay/OgreOverlayContainer.h>
#include <OgreSingleton.h>

//! \brief See: http://www.ogre3d.org/tikiwiki/Simple+Text+Output&structure=Cookbook
class TextRenderer: public Ogre::Singleton<TextRenderer>
{
public:
    TextRenderer();
    ~TextRenderer();

    void addTextBox(const std::string& ID, const std::string& text,
                    Ogre::Real x, Ogre::Real y, Ogre::Real width,
                    Ogre::Real height, const Ogre::ColourValue& color =
                    Ogre::ColourValue(1.0, 1.0, 1.0, 1.0));

    void removeTextBox(const std::string& ID);

    void setText(const std::string& ID, const std::string& Text);

    //! \brief Change the text color
    void setColor(const std::string& ID, const Ogre::ColourValue& color);

    void moveText(const std::string& ID, Ogre::Real left, Ogre::Real top);

private:
    Ogre::OverlayManager* mOverlayMgr;
    Ogre::Overlay* mOverlay;
    Ogre::OverlayContainer* mPanel;
};

#endif // TEXTRENDERER_H
