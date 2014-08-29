/*
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

#include "ConsoleMode.h"

#include "Gui.h"
#include "Console.h"
#include "LogManager.h"
#include "ASWrapper.h"
#include "RenderManager.h"
#include "PrefixTree.h"
#include "ODFrameListener.h"

#include <list>
#include <string>

const std::string CONSOLE_COMMANDS = "./config/console_commands.txt";

ConsoleMode::ConsoleMode(ModeManager* modeManager, Console* console):
    AbstractApplicationMode(modeManager, ModeManager::CONSOLE),
    mConsole(console),
    mPrefixTree(NULL),
    mLl(NULL),
    mNonTagKeyPressed(true)
{
    mPrefixTree = new PrefixTree();
    mLl = new list<string>();
    mPrefixTree->readStringsFromFile(CONSOLE_COMMANDS.c_str());
}

ConsoleMode::~ConsoleMode()
{
    delete mPrefixTree;
    delete mLl;
}

void ConsoleMode::activate()
{
    // Loads the corresponding Gui sheet.
    Gui::getSingleton().loadGuiSheet(Gui::hideGui);

    giveFocus();
}

bool ConsoleMode::mouseMoved(const OIS::MouseEvent &arg)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition((float)arg.state.X.abs, (float)arg.state.Y.abs);
    if(arg.state.Z.rel == 0 || !mConsole->mVisible)
        return false;

    if(getKeyboard()->isModifierDown(OIS::Keyboard::Ctrl))
        mConsole->scrollHistory(arg.state.Z.rel > 0);
    else
        mConsole->scrollText(arg.state.Z.rel > 0);

    mConsole->mUpdateOverlay = true;
    return true;
}

bool ConsoleMode::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id){
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
        Gui::getSingletonPtr()->convertButton(id));
}

bool ConsoleMode::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id){
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
        Gui::getSingletonPtr()->convertButton(id));
}

bool ConsoleMode::keyPressed(const OIS::KeyEvent &arg)
{
    if (!mConsole->mVisible)
        return false;

    if(arg.key == OIS::KC_TAB)
    {
        if(!mNonTagKeyPressed)
        {
            // it points to postfix candidate
            if (mIt == mLl->end())
            {
                mConsole->mPrompt = mPrefix;
                mIt = mLl->begin();
            }
            else{
                mConsole->mPrompt = mPrefix + *mIt;
                ++mIt;
            }
        }
        else
        {
            mLl->clear();
            mPrefixTree->complete(mConsole->mPrompt.c_str(), mLl);
            mPrefix = mConsole->mPrompt ;
            mIt = mLl->begin();
        }

        mNonTagKeyPressed= false;
    }
    else
    {
        mNonTagKeyPressed = true;
        switch(arg.key)
        {
        case OIS::KC_GRAVE:
        case OIS::KC_ESCAPE:
        case OIS::KC_F12:
            regressMode();
            mConsole->setVisible(false);
            ODFrameListener::getSingleton().setTerminalActive(false);
            break;

        case OIS::KC_RETURN:
        {
            //only do this for non-empty input
            if(!mConsole->mPrompt.empty())
            {
                //print our input and push it to the history
                mConsole->print(mConsole->mPrompt);
                mConsole->mHistory.push_back(mConsole->mPrompt);
                ++mConsole->mCurHistPos;

                //split the input into it's space-separated "words"
                std::vector<Ogre::String> params = mConsole->split(mConsole->mPrompt, ' ');

                //TODO: remove this until AS console handler is ready
                Ogre::String command = params[0];
                Ogre::String arguments;
                for(size_t i = 1; i< params.size(); ++i)
                {
                    arguments += params[i];
                    if(i < params.size() - 1)
                    {
                        arguments += ' ';
                    }
                }
                //remove until this point

                //TODO: remove executePromptCommand after it is fully converted
                //for now try hardcoded commands, and if none is found try AS
                if(!mConsole->executePromptCommand(command, arguments))
                {
                    LogManager::getSingleton().logMessage("Console command: " + command
                        + " - arguments: " + arguments + " - actionscript");
                    ASWrapper::getSingleton().executeConsoleCommand(params);
                }

                mConsole->mPrompt.clear();
            }
            else
            {
                // Set history position back to last entry
                mConsole->mCurHistPos = mConsole->mHistory.size();
            }
            break;
        }
        case OIS::KC_BACK:
            mConsole->mPrompt = mConsole->mPrompt.substr(0, mConsole->mPrompt.length() - 1);
            break;

        case OIS::KC_PGUP:
            mConsole->scrollText(true);
            break;

        case OIS::KC_PGDOWN:
            mConsole->scrollText(false);
            break;

        case OIS::KC_UP:
            mConsole->scrollHistory(true);
            break;

        case OIS::KC_DOWN:
            mConsole->scrollHistory(false);
            break;

        case OIS::KC_F10:
        {
            LogManager::getSingleton().logMessage("RTSS test----------");
            RenderManager::getSingleton().rtssTest();
            break;
        }

        default:
            if (std::string("abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ,.<>/?1234567890-=\\!@#$%^&*()_+|;\':\"[]{}").find(
                            arg.text) != std::string::npos)
            {
                mConsole->mPrompt += arg.text;
            }
            break;
        }
    }

    mConsole->mUpdateOverlay = true;
    return true;
}

bool ConsoleMode::keyReleased(const OIS::KeyEvent &arg)
{
    return true;
}

void ConsoleMode::handleHotkeys(OIS::KeyCode keycode)
{

}
