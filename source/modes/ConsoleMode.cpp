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

#include "modes/ConsoleCommands.h"
#include "render/Gui.h"
#include "utils/LogManager.h"

#include <CEGUI/widgets/Listbox.h>
#include <CEGUI/widgets/ListboxTextItem.h>
#include <CEGUI/widgets/PushButton.h>
#include <CEGUI/widgets/Editbox.h>
#include <CEGUI/widgets/Scrollbar.h>

#include <functional>
#include <cassert>

ConsoleMode::ConsoleMode(ModeManager* modeManager):
    AbstractApplicationMode(modeManager, ModeManager::CONSOLE),
    mConsoleInterface(std::bind(&ConsoleMode::printToConsole, this, std::placeholders::_1))
{
    ConsoleCommands::addConsoleCommands(mConsoleInterface);

    CEGUI::Window* consoleRootWindow = Gui::getSingleton().getGuiSheet(Gui::guiSheet::console);
    assert(consoleRootWindow != nullptr);
    CEGUI::Window* listbox = consoleRootWindow->getChild("ConsoleHistoryWindow");
    assert(listbox->getType().compare("OD/Listbox") == 0);
    mConsoleHistoryWindow = static_cast<CEGUI::Listbox*>(listbox);
    CEGUI::Window* editbox = consoleRootWindow->getChild("Editbox");
    mEditboxWindow = static_cast<CEGUI::Editbox*>(editbox);
    CEGUI::Window* sendButton = consoleRootWindow->getChild("SendButton");
    sendButton->subscribeEvent(CEGUI::PushButton::EventClicked,
                               CEGUI::Event::Subscriber(&ConsoleMode::executeCurrentPrompt, this));
    mEditboxWindow->subscribeEvent(CEGUI::Editbox::EventTextAccepted,
                                   CEGUI::Event::Subscriber(&ConsoleMode::executeCurrentPrompt, this));
    //TODO: This should be done in the xml file if possible.
    mConsoleHistoryWindow->getVertScrollbar()->setEndLockEnabled(true);
    subscribeCloseButton(*consoleRootWindow);
}

ConsoleMode::~ConsoleMode()
{
}

void ConsoleMode::activate()
{
    // Loads the corresponding Gui sheet.
    Gui::getSingleton().loadGuiSheet(Gui::console);
    mEditboxWindow->activate();
    giveFocus();
}

bool ConsoleMode::keyPressed(const OIS::KeyEvent &arg)
{
    switch(arg.key)
    {
        case OIS::KC_TAB:
        {
            if(auto completed = mConsoleInterface.tryCompleteCommand(mEditboxWindow->getText().c_str()))
            {
                mEditboxWindow->setText(completed.get());
            }
            mEditboxWindow->setCaretIndex(mEditboxWindow->getText().length());
            break;
        }
        case OIS::KC_GRAVE:
        case OIS::KC_ESCAPE:
        case OIS::KC_F12:
        {
            regressMode();
            break;
        }
        case OIS::KC_UP:
            if(auto completed = mConsoleInterface.scrollCommandHistoryPositionUp(mEditboxWindow->getText().c_str()))
            {
                mEditboxWindow->setText(completed.get());
            }
            mEditboxWindow->setCaretIndex(mEditboxWindow->getText().length());
            break;

        case OIS::KC_DOWN:
        {
            if(auto completed = mConsoleInterface.scrollCommandHistoryPositionDown())
            {
                mEditboxWindow->setText(completed.get());
            }
            mEditboxWindow->setCaretIndex(mEditboxWindow->getText().length());
            break;
        }
        default:
        {
            CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyDown(
                static_cast<CEGUI::Key::Scan>(arg.key));
            CEGUI::System::getSingleton().getDefaultGUIContext().injectChar(
                static_cast<CEGUI::String::value_type>(arg.text));
            break;
        }
    }

    return true;
}

void ConsoleMode::printToConsole(const std::string& text)
{
    mConsoleHistoryWindow->addItem(new CEGUI::ListboxTextItem(text));
}

bool ConsoleMode::executeCurrentPrompt(const CEGUI::EventArgs &e)
{
    mConsoleInterface.tryExecuteCommand(mEditboxWindow->getText().c_str(),
                                    getModeManager().getCurrentModeTypeExceptConsole(),
                                    getModeManager());
    mEditboxWindow->setText("");
    return true;
}
