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

#include "GameEditorModeConsole.h"

#include "modes/ConsoleCommands.h"
#include "render/Gui.h"
#include "utils/LogManager.h"
#include "modes/GameEditorModeBase.h"

#include <CEGUI/widgets/Listbox.h>
#include <CEGUI/widgets/ListboxTextItem.h>
#include <CEGUI/widgets/PushButton.h>
#include <CEGUI/widgets/Editbox.h>
#include <CEGUI/widgets/Scrollbar.h>

#include <functional>
#include <cassert>

GameEditorModeConsole::GameEditorModeConsole(ModeManager* modeManager):
    mConsoleInterface(std::bind(&GameEditorModeConsole::printToConsole, this, std::placeholders::_1)),
    mModeManager(modeManager)
{
    ConsoleCommands::addConsoleCommands(mConsoleInterface);

    CEGUI::Window* consoleRootWindow = mModeManager->getGui().getGuiSheet(Gui::guiSheet::console);
    assert(consoleRootWindow != nullptr);
    CEGUI::Window* listbox = consoleRootWindow->getChild("ConsoleHistoryWindow");
    assert(listbox->getType().compare("OD/Listbox") == 0);
    mConsoleHistoryWindow = static_cast<CEGUI::Listbox*>(listbox);
    CEGUI::Window* editbox = consoleRootWindow->getChild("Editbox");
    mEditboxWindow = static_cast<CEGUI::Editbox*>(editbox);
    CEGUI::Window* sendButton = consoleRootWindow->getChild("SendButton");

    addEventConnection(
        sendButton->subscribeEvent(CEGUI::PushButton::EventClicked,
                                   CEGUI::Event::Subscriber(&GameEditorModeConsole::executeCurrentPrompt, this))
    );

    mConsoleHistoryWindow->getVertScrollbar()->setEndLockEnabled(true);

    // Permits closing the console.
    CEGUI::Window* closeButton = consoleRootWindow->getChild("__auto_closebutton__");
    addEventConnection(
        closeButton->subscribeEvent(CEGUI::PushButton::EventClicked,
                                    CEGUI::Event::Subscriber(&GameEditorModeConsole::leaveConsole, this))
    );
}

GameEditorModeConsole::~GameEditorModeConsole()
{
    //Disconnect all event connections.
    for(CEGUI::Event::Connection& c : mEventConnections)
    {
        c->disconnect();
    }
}

void GameEditorModeConsole::activate()
{
    // Loads the corresponding Gui sheet.
    mModeManager->getGui().loadGuiSheet(Gui::console);
    mEditboxWindow->activate();
}

bool GameEditorModeConsole::keyPressed(const OIS::KeyEvent &arg)
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
            leaveConsole();
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
        case OIS::KC_RETURN:
        case OIS::KC_NUMPADENTER:
            executeCurrentPrompt();
            break;
        default:
            break;
    }

    return true;
}

void GameEditorModeConsole::printToConsole(const std::string& text)
{
    mConsoleHistoryWindow->addItem(new CEGUI::ListboxTextItem(text));
}

bool GameEditorModeConsole::executeCurrentPrompt(const CEGUI::EventArgs& e)
{
    mConsoleInterface.tryExecuteCommand(mEditboxWindow->getText().c_str(),
                                        mModeManager->getCurrentModeType(),
                                        *mModeManager);
    mEditboxWindow->setText("");
    return true;
}

bool GameEditorModeConsole::leaveConsole(const CEGUI::EventArgs& /*e*/)
{
    if (mModeManager->getCurrentModeType() != AbstractModeManager::GAME
        && mModeManager->getCurrentModeType() != AbstractModeManager::EDITOR)
        return true;

    // Warn the mother mode that we can leave the console.
    GameEditorModeBase* mode = static_cast<GameEditorModeBase*>(mModeManager->getCurrentMode());
    mode->leaveConsole();
    return true;
}
