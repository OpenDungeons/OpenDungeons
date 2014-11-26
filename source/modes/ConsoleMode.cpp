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

#include "render/Gui.h"
#include "utils/LogManager.h"
//#include "scripting/ASWrapper.h"
#include "render/RenderManager.h"
#include "sound/SoundEffectsManager.h"

#include <CEGUI/widgets/PushButton.h>

#include <functional>
#include <list>
#include <string>

const std::string CONSOLE_COMMANDS = "./config/console_commands.txt";

ConsoleMode::ConsoleMode(ModeManager* modeManager):
    AbstractApplicationMode(modeManager, ModeType::CONSOLE),
    mConsoleHistoryWindow(nullptr),
    mEditBoxWindow(nullptr),
    mConsoleInterface(std::bind(&ConsoleMode::printToWindow, this, std::placeholders::_1))
{
    CEGUI::Window* consoleRootWindow = modeManager->getGui()->getGuiSheet(Gui::guiSheet::console);
    if(consoleRootWindow != nullptr)
    {
        try
        {
            mConsoleHistoryWindow = consoleRootWindow->getChild("ConsoleHistoryWindow");
            mEditBoxWindow = consoleRootWindow->getChild("Editbox");
            CEGUI::Window* sendButton = consoleRootWindow->getChild("SendButton");
            sendButton->subscribeEvent(CEGUI::PushButton::EventClicked,
                                       CEGUI::Event::Subscriber(&ConsoleMode::sendButtonClick, this));
        }
        catch(const CEGUI::Exception& e)
        {
            LogManager::getSingleton().logMessage(std::string("ERROR: Failed to create console window. Exception: ") + e.what(),
                                                  Ogre::LogMessageLevel::LML_CRITICAL);
        }
    }
    else
    {
        LogManager::getSingleton().logMessage("ERROR: Could not find console GUI sheet",
                                              Ogre::LogMessageLevel::LML_CRITICAL);
    }
}

ConsoleMode::~ConsoleMode()
{
}

void ConsoleMode::activate()
{
    LogManager::getSingleton().logMessage("Activating console");
    // Loads the corresponding Gui sheet.
    Gui::getSingleton().loadGuiSheet(Gui::console);
    giveFocus();
}

bool ConsoleMode::mouseMoved(const OIS::MouseEvent &arg)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition(
                static_cast<float>(arg.state.X.abs), static_cast<float>(arg.state.Y.abs));
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
    CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyDown((CEGUI::Key::Scan) arg.key);
    CEGUI::System::getSingleton().getDefaultGUIContext().injectChar(arg.text);

    if(!mEditBoxWindow || !mConsoleHistoryWindow)
    {
        LogManager::getSingleton().logMessage("Error: Console windows are not initialised.");
        return false;
    }

    switch(arg.key)
    {
        case OIS::KC_TAB:
        {
            const CEGUI::String& cmd = mEditBoxWindow->getText();
            auto completedCommand = mConsoleInterface.tryCompleteCommand(cmd.c_str());
            if(completedCommand)
            {
                mEditBoxWindow->setText(*completedCommand);
            }
            break;
        }
        case OIS::KC_GRAVE:
        case OIS::KC_ESCAPE:
        case OIS::KC_F12:
        {
            regressMode();
            break;
        }
        case OIS::KC_RETURN:
        {
            if(mEditBoxWindow->isActive())
            {
                const CEGUI::String& cmd = mEditBoxWindow->getText();
                executeCommand(cmd);
                mEditBoxWindow->setText("");
            }
            break;
        }
        case OIS::KC_BACK:
            break;

        case OIS::KC_PGUP:
            break;

        case OIS::KC_PGDOWN:
            break;

        case OIS::KC_UP:
        {
            const CEGUI::String& currentCommand = mEditBoxWindow->getText();
            auto scrolledCommand = mConsoleInterface.scrollCommandHistoryPositionUp(currentCommand.c_str());
            if(scrolledCommand)
            {
                mEditBoxWindow->setText(*scrolledCommand);
            }
            break;
        }
        case OIS::KC_DOWN:
        {
            auto scrolledCommand = mConsoleInterface.scrollCommandHistoryPositionDown();
            if(scrolledCommand)
            {
                mEditBoxWindow->setText(*scrolledCommand);
            }
            break;
        }
        default:
            break;
    }
    return true;
}

bool ConsoleMode::keyReleased(const OIS::KeyEvent &arg)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyUp((CEGUI::Key::Scan) arg.key);
    return true;
}

void ConsoleMode::handleHotkeys(OIS::KeyCode keycode)
{

}

bool ConsoleMode::sendButtonClick(const CEGUI::EventArgs &e)
{
    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    executeCommand(mEditBoxWindow->getText());
    return true;
}

void ConsoleMode::executeCommand(const CEGUI::String &command)
{
    mConsoleInterface.tryExecuteCommand(command.c_str(), ModeType::CONSOLE, mModeManager);
}

void ConsoleMode::printToWindow(const std::string& string)
{
    if(mConsoleHistoryWindow)
    {
        mConsoleHistoryWindow->appendText(string);
    }
}
