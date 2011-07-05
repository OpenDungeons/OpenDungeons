/*!
 * \file   Console.h
 * \date:  04 July 2011
 * \author StefanP.MUC
 * \brief  Ingame console
 */

#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <list>
#include <vector>

#include <OgreFrameListener.h>
#include <Ogre.h>
#include <OIS/OIS.h>

#include "Gui.h"

class Console :
        public Ogre::Singleton<Console>,
        public Ogre::FrameListener
        //public Ogre::LogListener
{
    public:
        Console();
        ~Console();

        inline const bool& isVisible() const{return visible;}
        void setVisible(const bool& newState);
        void toggleVisibility();

        void print(const Ogre::String &text);

        virtual bool frameStarted(const Ogre::FrameEvent &evt);
        virtual bool frameEnded(const Ogre::FrameEvent &evt);

        void onKeyPressed(const OIS::KeyEvent &arg);

        void addCommand(const Ogre::String &command, void (*)(std::vector<Ogre::String>&));
        void removeCommand(const Ogre::String &command);

        //void messageLogged(const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName) {print(logName + ": " + message);}

    private:
        unsigned int consoleLineLength;
        unsigned int consoleLineCount;
        bool visible;

        Ogre::OverlayContainer* panel;
        Ogre::OverlayElement*   textbox;
        Ogre::Overlay*          overlay;

        bool updateOverlay;
        int startLine;
        std::list<Ogre::String> lines;
        Ogre::String prompt;
        std::map<Ogre::String, void(*)(std::vector<Ogre::String>&)> commands;

        void checkVisibility();
};

#endif /* CONSOLE_H_ */
