/*!
 * \file   Console.cpp
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

/* TODO: do intense testing that everything works
 * TODO: switch from TextRenderer to Console
 */

#include "Console.h"

#include "ASWrapper.h"

#include "GameMode.h"
#include "LogManager.h"
#include "ODApplication.h"
#include "ODFrameListener.h"
#include "RenderManager.h"

#include "ModeManager.h"

#include <Overlay/OgreOverlayManager.h>

template<> Console* Ogre::Singleton<Console>::msSingleton = 0;

Console::Console() :
    mModeManager(NULL),
    mCm(NULL),
    mOdf(NULL),
    //these two define how much text goes into the console
    mConsoleLineLength(100),
    mConsoleLineCount(14),
    mBlinkSpeed(0.5),
    mTimeSinceLastBlink(0.0),
    mVisible(false),
    mUpdateOverlay(true),
    mAllowTrivial(false),
    mAllowNormal(false),
    mAllowCritical(true),
    mChatMode(false),
    mCursorVisible(true),
    mPanel(NULL),
    mTextbox(NULL),
    mOverlay(NULL),
    mStartLine(0),
    mCursorChar("_"),
    mCurHistPos(0)
{
    LogManager::getSingleton().logMessage("*** Initiliasing Console ***");
    ODApplication::getSingleton().getRoot()->addFrameListener(this);
    Ogre::OverlayManager& olMgr = Ogre::OverlayManager::getSingleton();

    // Create a panel
    mPanel = static_cast<Ogre::OverlayContainer*>(
            olMgr.createOverlayElement("Panel", "ConsolePanel"));
    mPanel->setPosition(0, 0.7);
    mPanel->setDimensions(1, 0.3);
    mPanel->setMaterialName("console/background");

    // Create a text area
    mTextbox = olMgr.createOverlayElement("TextArea", "ConsoleText");
    mTextbox->setPosition(0, 0);
    mTextbox->setParameter("font_name", "FreeMono");
    mTextbox->setParameter("char_height", "0.02");

    // Create an overlay, and add the panel
    mOverlay = olMgr.create("Console");
    mOverlay->add2D(mPanel);

    // Add the text area to the panel
    mPanel->addChild(mTextbox);

    LogManager::getSingleton().getLog().addListener(this);
}

Console::~Console()
{
    delete mPanel;
    delete mTextbox;
    delete mOverlay;
}

/*! \brief Defines the action on starting the current frame
 *
 *  The Console listener checks if it needs updating and if it does it will
 *  redraw itself with the new text
 */
bool Console::frameStarted(const Ogre::FrameEvent& evt)
{
    if(mVisible)
    {
        mTimeSinceLastBlink += evt.timeSinceLastFrame;

        if(mTimeSinceLastBlink >= mBlinkSpeed)
        {
            mTimeSinceLastBlink -= mBlinkSpeed;
            mCursorVisible = !mCursorVisible;
            mUpdateOverlay = true;
        }
    }

    if(mUpdateOverlay)
    {
        Ogre::String text;
        std::list<Ogre::String>::iterator it, start, end;

        //make sure is in range
        if(mStartLine > mLines.size())
        {
            mStartLine = mLines.size();
        }

        start = mLines.begin();
        for (unsigned int c = 0; c < mStartLine; ++c)
        {
            ++start;
        }

        end = start;
        for (unsigned int c = 0; c < mConsoleLineCount; ++c)
        {
            if (end == mLines.end())
            {
                break;
            }
            ++end;
        }

        unsigned int counter = 0;
        for (it = start; it != end; ++it)
        {
            text += (*it) + "\n";
            ++counter;
        }

        for(; counter < mConsoleLineCount; ++counter)
        {
            text += "\n";
        }
        //add the prompt
        text += ">>> " + mPrompt + (mCursorVisible ? mCursorChar : "");

        mTextbox->setCaption(text);
        mUpdateOverlay = false;
    }

    return true;
}

/*! \brief what happens after frame
 *
 */
bool Console::frameEnded(const Ogre::FrameEvent& evt)
{
    return true;
}

/*! \brief print text to the console
 *
 * This function automatically checks if there are linebreaks in the text
 * and separates the text into separate strings
 *
 * \param text The text to be added to the console
 */
void Console::print(const Ogre::String& text)
{
    std::vector<Ogre::String> newLines = split(text, '\n');
    mLines.insert(mLines.end(), newLines.begin(), newLines.end());

    mStartLine = (mLines.size() > mConsoleLineCount)
                            ? mLines.size() - mConsoleLineCount
                            : 0;

    mUpdateOverlay = true;
}

/*! \brief show or hide the console manually
 *
 */
void Console::setVisible(const bool newState)
{
    mVisible = newState;
    Gui::getSingleton().setVisible(!mVisible);
    checkVisibility();
}

/*! \brief enables or disables the console, depending on what state it has
 *
 */
void Console::toggleVisibility()
{
    mVisible = !mVisible;
    Gui::getSingleton().setVisible(!mVisible);
    checkVisibility();
}

/*! \brief Does the actual showing/hiding depending on bool visible
 *
 */
void Console::checkVisibility()
{
    if(mVisible)
    {
        mOverlay->show();
    }
    else
    {
        mOverlay->hide();
    }
}

/*! \brief Splits a string on every occurance of splitChar
 *
 *  \return A vector of all splitted sub strings
 *
 *  \param str The string to be splitted
 *  \param splitChar The character that defines the split positions
 */
std::vector<Ogre::String> Console::split(const Ogre::String& str, const char splitChar)
{
    std::vector<Ogre::String> splittedStrings;
    size_t lastPos = 0, pos = 0;
    do
    {
        pos = str.find(splitChar, lastPos);
        splittedStrings.push_back(str.substr(lastPos, pos - lastPos));
        lastPos = pos + 1; //next time start AFTER the last space
    }
    while(pos != std::string::npos);

    return splittedStrings;
}

/*! \brief Send logged messages also to the Console
 *
 * We only allow critical messages to the console. Non-critical messages would
 * pollute the console window and make it hardly readable.
 */
void Console::messageLogged(const Ogre::String& message, Ogre::LogMessageLevel lml,
                            bool maskDebug, const Ogre::String& logName, bool& skipThisMessage)
{
    // if skipThisMessage is true then just return, skipping the rest of the implementation
    if(skipThisMessage)
        return;

    //test if the logLevel is allowed, if not then return
    switch(lml)
    {
        case Ogre::LML_CRITICAL:
            if(!mAllowCritical)
                return;
            break;

        case Ogre::LML_TRIVIAL:
            if(!mAllowTrivial)
                return;
            break;

        case Ogre::LML_NORMAL:
            if(!mAllowNormal)
                return;
            break;

        default:
            return;
    }

    //if it was allowed then print the message
    print(logName + ": " + message);
}

/*! \brief Scrolls through the history of user entered commands
 *
 *  \param direction true means going up (old), false means going down (new)
 */
void Console::scrollHistory(const bool direction)
{
    if(direction)
    {
        //don't go unter 0, it's an unsigned int and the minimum index!
        if(mCurHistPos == 0)
        {
            return;
        }
        else
        {
            --mCurHistPos;
        }
    }
    else
    {
        //don't go over maximum index and clear the prompt when trying.
        if(++mCurHistPos >= mHistory.size())
        {
            mCurHistPos = mHistory.size();
            mPrompt = "";
            return;
        }

    }

    mPrompt = mHistory[mCurHistPos];
}

/*! \brief Scrolls through the text output in the console
 *
 *  \param direction true means going up (old), false means going down (new)
 */
void Console::scrollText(const bool direction)
{
    if(direction)
    {
        if(mStartLine > 0)
        {
            --mStartLine;
        }
    }
    else
    {
        if(mStartLine < mLines.size() && mLines.size() - mStartLine > mConsoleLineCount)
        {
            ++mStartLine;
        }
    }
}
