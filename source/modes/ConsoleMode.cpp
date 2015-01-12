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

#include "ConsoleMode.h"

#include "modes/Console.h"
#include "modes/ConsoleCommands.h"
#include "render/RenderManager.h"
#include "render/ODFrameListener.h"
#include "render/Gui.h"
#include "utils/LogManager.h"
#include "utils/Helper.h"

#include <functional>

ConsoleMode::ConsoleMode(ModeManager* modeManager, Console* console):
    AbstractApplicationMode(modeManager, ModeManager::CONSOLE),
    mConsole(console),
    mConsoleInterface(std::bind(&ConsoleMode::printToConsole, this, std::placeholders::_1))
{
    ConsoleCommands::addConsoleCommands(mConsoleInterface);
}

ConsoleMode::~ConsoleMode()
{
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
        if(auto completed = mConsoleInterface.tryCompleteCommand(mConsole->mPrompt))
        {
            mConsole->mPrompt = *completed;
        }
    }
    else
    {
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

                mConsoleInterface.tryExecuteCommand(mConsole->mPrompt,
                                                    getModeManager().getCurrentModeTypeExceptConsole(),
                                                    getModeManager());

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

void ConsoleMode::printToConsole(const std::string& text)
{
    //print our input and push it to the history
    mConsole->print(text);
    mConsole->mHistory.push_back(text);
    ++mConsole->mCurHistPos;
}
