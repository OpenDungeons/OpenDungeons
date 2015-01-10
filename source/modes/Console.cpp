/*!
 * \file   Console.cpp
 * \date:  04 July 2011
 * \author StefanP.MUC
 * \brief  Ingame console
 *
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

#include "modes/Console.h"

#include "modes/GameMode.h"
#include "render/RenderManager.h"

#include "utils/LogManager.h"
#include "utils/Helper.h"

#include <Overlay/OgreOverlayManager.h>
#include <OgreRoot.h>

template<> Console* Ogre::Singleton<Console>::msSingleton = 0;

Console::Console() :
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
    mPanel(nullptr),
    mTextbox(nullptr),
    mOverlay(nullptr),
    mStartLine(0),
    mCursorChar("_"),
    mCurHistPos(0)
{
    LogManager::getSingleton().logMessage("*** Initializing Console ***");
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

    Ogre::Root::getSingleton().addFrameListener(this);
    LogManager::getSingleton().getLog().addListener(this);
}

Console::~Console()
{
    LogManager::getSingleton().logMessage("*** Deinitializing Console ***");
    LogManager::getSingleton().getLog().removeListener(this);
    Ogre::Root::getSingleton().removeFrameListener(this);
    // Handled by Ogre.
    //delete mPanel;
    //delete mTextbox;
    //delete mOverlay;
}

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
        //make sure is in range
        if(mStartLine > mLines.size())
        {
            mStartLine = mLines.size();
        }

        std::vector<std::string>::iterator start = mLines.begin();
        for (unsigned int c = 0; c < mStartLine; ++c)
        {
            ++start;
        }

        std::vector<std::string>::iterator end = start;
        for (unsigned int c = 0; c < mConsoleLineCount; ++c)
        {
            if (end == mLines.end())
            {
                break;
            }
            ++end;
        }

        unsigned int counter = 0;
        std::string text;
        for (std::vector<std::string>::iterator it = start; it != end; ++it)
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

bool Console::frameEnded(const Ogre::FrameEvent& evt)
{
    return true;
}

void Console::print(const std::string& text)
{
    std::vector<std::string> newLines = Helper::split(text, '\n');
    mLines.insert(mLines.end(), newLines.begin(), newLines.end());

    mStartLine = (mLines.size() > mConsoleLineCount)
                            ? mLines.size() - mConsoleLineCount
                            : 0;

    mUpdateOverlay = true;
}

void Console::setVisible(const bool newState)
{
    mVisible = newState;
    checkVisibility();
}

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

void Console::messageLogged(const std::string& message, Ogre::LogMessageLevel lml,
                            bool maskDebug, const std::string& logName, bool& skipThisMessage)
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
