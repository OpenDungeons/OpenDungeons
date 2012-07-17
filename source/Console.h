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
#include <ODFrameListener.h>
#include <Ogre.h>
#include <OIS/OIS.h>
#include <GameMode.h>
#include "GameMap.h"

#include "Gui.h"

using std::string;
class ModeManager;


class Console :
        public Ogre::Singleton<Console>,
        public Ogre::FrameListener,
        public Ogre::LogListener
{
    friend class ConsoleMode;
    public:
        Console();
        ~Console();

	void  setModeManager(ModeManager* mm){modeManager = mm;};

        inline const bool& isVisible() const{return visible;}
        void setVisible(const bool newState);
        void toggleVisibility();

        inline const bool&  getAllowTrivial() const                 { return allowTrivial; }
        inline void         setAllowTrivial(const bool& newState)   { allowTrivial = newState; }

        inline const bool&  getAllowNormal() const                  { return allowNormal; }
        inline void         setAllowNormal(const bool newState)     { allowNormal = newState; }

        inline const bool&  getAllowCritical() const                { return allowCritical; }
        inline void         setAllowCritical(const bool newState)   { allowCritical = newState; }

        inline const bool&  getChatMode() const                     { return chatMode; }
        inline void         setChatMode(const bool newState)        { chatMode = newState; }

        void print(const Ogre::String &text);

        virtual bool frameStarted   (const Ogre::FrameEvent& evt);
        virtual bool frameEnded     (const Ogre::FrameEvent& evt);

        void onMouseMoved   (const OIS::MouseEvent& arg, const bool isCtrlDown = false);
        void onKeyPressed   (const OIS::KeyEvent& arg);
        void messageLogged  (const Ogre::String& message, Ogre::LogMessageLevel lml,
        bool maskDebug, const Ogre::String& logName);
        bool executePromptCommand(const std::string& command, std::string arguments);
	string getHelpText(std::string arg);
	void printText(const std::string& text);

    private:

        // Console variables
        std::deque<ChatMessage*> chatMessages;
        std::string promptCommand, chatString;



        //state variables

	ModeManager*    modeManager;
	ODFrameListener* odf;
        unsigned int    consoleLineLength;
        unsigned int    consoleLineCount;
        Ogre::Real      blinkSpeed;
        Ogre::Real      timeSinceLastBlink;
        bool            visible;
        bool            updateOverlay;
        bool            allowTrivial;
        bool            allowNormal;
        bool            allowCritical;
        bool            chatMode;
        bool            cursorVisible;

        //basic conatiner objects
        Ogre::OverlayContainer* panel;
        Ogre::OverlayElement*   textbox;
        Ogre::Overlay*          overlay;

        //input/output storage variakes
        unsigned int            startLine;
        std::list<Ogre::String> lines;
        Ogre::String            prompt;
        Ogre::String            cursorChar;

        //history variables
        std::vector<Ogre::String>   history;
        unsigned int                curHistPos;

        void                        checkVisibility ();
        std::vector<Ogre::String>   split           (const Ogre::String& str,
                const char splitChar);
        void                        scrollHistory   (const bool direction);
        void                        scrollText      (const bool direction);
};

#endif /* CONSOLE_H_ */
