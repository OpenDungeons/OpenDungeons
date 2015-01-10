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

#include "modes/GameMode.h"
#include "gamemap/GameMap.h"
#include "render/Gui.h"

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

#include <vector>

class Console :
    public Ogre::Singleton<Console>,
    public Ogre::FrameListener,
    public Ogre::LogListener
{
    friend class ConsoleMode;

public:
    Console();
    ~Console();

    inline bool isVisible() const
    { return mVisible; }

    //! \brief show or hide the console manually
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

    /*! \brief print text to the console
     *
     * This function automatically checks if there are linebreaks in the text
     * and separates the text into separate strings
     *
     * \param text The text to be added to the console
     */
    void print(const std::string& text);

    /*! \brief Defines the action on starting the current frame
     *
     *  The Console listener checks if it needs updating and if it does it will
     *  redraw itself with the new text
     */
    virtual bool frameStarted (const Ogre::FrameEvent& evt);

    //! \brief what happens after frame
    virtual bool frameEnded (const Ogre::FrameEvent& evt);

    void onMouseMoved (const OIS::MouseEvent& arg, const bool isCtrlDown = false);
    void onKeyPressed (const OIS::KeyEvent& arg);

    /*! \brief Send logged messages also to the Console
     *
     * We only allow critical messages to the console. Non-critical messages would
     * pollute the console window and make it hardly readable.
     */
    void messageLogged (const std::string& message, Ogre::LogMessageLevel lml,
                        bool maskDebug, const std::string& logName, bool& skipThisMessage);

private:
    std::string mPromptCommand;
    std::string mChatString;

    //! \brief State variables
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

    //! \brief Basic container objects
    Ogre::OverlayContainer* mPanel;
    Ogre::OverlayElement* mTextbox;
    Ogre::Overlay* mOverlay;

    //! \brief Input/output storage variables
    unsigned int mStartLine;
    std::vector<std::string> mLines;
    std::string mPrompt;
    std::string mCursorChar;

    //! \brief Text/Commands history variables
    std::vector<std::string> mHistory;
    unsigned int mCurHistPos;

    //! \brief Does the actual showing/hiding depending on bool visible
    void checkVisibility();

    /*! \brief Scrolls through the history of user entered commands
     *
     *  \param direction true means going up (old), false means going down (new)
     */
    void scrollHistory(const bool direction);

    /*! \brief Scrolls through the text output in the console
     *
     *  \param direction true means going up (old), false means going down (new)
     */
    void scrollText(const bool direction);
};

#endif // CONSOLE_H_
