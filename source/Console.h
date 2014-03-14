/*!
 * \file   Console.h
 * \date:  04 July 2011
 * \author StefanP.MUC
 * \brief  Ingame console
 *
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#ifndef CONSOLE_H_
#define CONSOLE_H_

#include "GameMode.h"
#include "GameMap.h"
#include "Gui.h"

#include <ODFrameListener.h>
#include <OIS/OISMouse.h>
#include <OIS/OISKeyboard.h>
#include <OgreSingleton.h>
#include <OgreString.h>
#include <OgreFrameListener.h>
#include <OgreLog.h>
#include <OgrePrerequisites.h>
#include <Overlay/OgreOverlayContainer.h>
#include <Overlay/OgreOverlay.h>
#include <Overlay/OgreOverlayElement.h>

#include <list>
#include <vector>

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

    inline const bool& isVisible() const
    { return mVisible; }

    void setVisible(const bool newState);

    inline const bool& getAllowTrivial() const
    { return mAllowTrivial; }

    inline void setAllowTrivial(const bool& newState)
    { mAllowTrivial = newState; }

    inline const bool& getAllowNormal() const
    { return mAllowNormal; }

    inline void setAllowNormal(const bool newState)
    { mAllowNormal = newState; }

    inline const bool& getAllowCritical() const
    { return mAllowCritical; }

    inline void setAllowCritical(const bool newState)
    { mAllowCritical = newState; }

    inline const bool& getChatMode() const
    { return mChatMode; }

    inline void setChatMode(const bool newState)
    { mChatMode = newState; }

    void print(const Ogre::String &text);

    virtual bool frameStarted (const Ogre::FrameEvent& evt);
    virtual bool frameEnded (const Ogre::FrameEvent& evt);

    void onMouseMoved (const OIS::MouseEvent& arg, const bool isCtrlDown = false);
    void onKeyPressed (const OIS::KeyEvent& arg);
    void messageLogged (const Ogre::String& message, Ogre::LogMessageLevel lml,
                        bool maskDebug, const Ogre::String& logName, bool& skipThisMessage);
    bool executePromptCommand(const std::string& command, std::string arguments);
    std::string getHelpText(std::string arg);
    void printText(const std::string& text);

private:
    // Console variables
    std::deque<ChatMessage*> mChatMessages;
    std::string mPromptCommand;
    std::string mChatString;

    //state variables
    unsigned int mConsoleLineLength;
    unsigned int mConsoleLineCount;
    Ogre::Real mBlinkSpeed;
    Ogre::Real mTimeSinceLastBlink;

    bool mVisible;
    bool mUpdateOverlay;
    bool mAllowTrivial;
    bool mAllowNormal;
    bool mAllowCritical;
    bool mChatMode;
    bool mCursorVisible;

    // Basic conatiner objects
    Ogre::OverlayContainer* mPanel;
    Ogre::OverlayElement* mTextbox;
    Ogre::Overlay* mOverlay;

    // Input/output storage variakes
    unsigned int mStartLine;
    std::list<Ogre::String> mLines;
    Ogre::String mPrompt;
    Ogre::String mCursorChar;

    // History variables
    std::vector<Ogre::String> mHistory;
    unsigned int mCurHistPos;

    void checkVisibility();
    std::vector<Ogre::String> split (const Ogre::String& str, const char splitChar);
    void scrollHistory (const bool direction);
    void scrollText (const bool direction);
};

#endif // CONSOLE_H_
