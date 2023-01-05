/*
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

#include "render/TextRenderer.h"

#include "utils/Helper.h"

#include <Overlay/OgreOverlayManager.h>
#include <Overlay/OgreOverlay.h>
#include <Overlay/OgreOverlayContainer.h>

template<> TextRenderer* Ogre::Singleton<TextRenderer>::msSingleton = nullptr;

TextRenderer::TextRenderer() :
        mOverlayMgr(Ogre::OverlayManager::getSingletonPtr()),
        mOverlay(mOverlayMgr->create("overlay1")),
        mPanel(static_cast<Ogre::OverlayContainer*>(mOverlayMgr->createOverlayElement("Panel", "container1")))
{
    mPanel->setDimensions(1, 1);
    mPanel->setPosition(0, 0);

    mOverlay->add2D(mPanel);
    mOverlay->show();
}

TextRenderer::~TextRenderer()
{
    // note: Ogre does this.
    //mOverlayMgr->destroyOverlayElement(mPanel);
    //mOverlayMgr->destroy(mOverlay);
}

void TextRenderer::addTextBox(const std::string& ID, const std::string& text,
                              Ogre::Real x, Ogre::Real y, Ogre::Real width, Ogre::Real height,
                              const Ogre::ColourValue& color)
{
    Ogre::OverlayElement* textBox = mOverlayMgr->createOverlayElement("TextArea", ID);
    textBox->setDimensions(width, height);
    textBox->setMetricsMode(Ogre::GMM_PIXELS);
    textBox->setPosition(x, y);
    textBox->setParameter("font_name", "MedievalSharp");
    textBox->setParameter("char_height", "16");
    textBox->setColour(color);

    textBox->setCaption(text);

    mPanel->addChild(textBox);
}

void TextRenderer::removeTextBox(const std::string& ID)
{
    mPanel->removeChild(ID);
    mOverlayMgr->destroyOverlayElement(ID);
}

void TextRenderer::setText(const std::string& ID, const std::string& text)
{
    Ogre::OverlayElement* textBox = mOverlayMgr->getOverlayElement(ID);
    // Note: We make ogre handle an UTF8 string
    if (textBox != nullptr)
        textBox->setCaption(text);
}

void TextRenderer::setColor(const std::string& ID, const Ogre::ColourValue& color)
{
    Ogre::OverlayElement* textBox = mOverlayMgr->getOverlayElement(ID);
    if (textBox != nullptr)
        textBox->setColour(color);
}

void TextRenderer::moveText(const std::string& ID, Ogre::Real left, Ogre::Real top)
{
    Ogre::OverlayElement* textBox = mPanel->getChild(ID);
    if (textBox != nullptr)
        textBox->setPosition(left, top);
}
