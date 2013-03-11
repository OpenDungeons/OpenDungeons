#ifndef TEXTRENDERER_H
#define TEXTRENDERER_H

#include <OgreOverlayManager.h>
#include <OgreOverlay.h>
#include <OgreOverlayContainer.h>
#include <OgreSingleton.h>

//http://www.ogre3d.org/tikiwiki/Simple+Text+Output&structure=Cookbook
class TextRenderer: public Ogre::Singleton<TextRenderer>
{
    private:
        Ogre::OverlayManager* _overlayMgr;
        Ogre::Overlay* _overlay;
        Ogre::OverlayContainer* _panel;

    public:
        TextRenderer();
        ~TextRenderer();

        void addTextBox(const std::string& ID, const std::string& text,
                Ogre::Real x, Ogre::Real y, Ogre::Real width,
                Ogre::Real height, const Ogre::ColourValue& color =
                        Ogre::ColourValue(1.0, 1.0, 1.0, 1.0));

        void removeTextBox(const std::string& ID);

        void setText(const std::string& ID, const std::string& Text);

        void moveText(const std::string& ID, Ogre::Real left, Ogre::Real top);
};

#endif

