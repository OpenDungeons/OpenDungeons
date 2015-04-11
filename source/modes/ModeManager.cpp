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

#include "modes/ModeManager.h"

#include "modes/InputManager.h"
#include "modes/MenuMode.h"
#include "modes/MenuModeConfigureSeats.h"
#include "modes/MenuModeSkirmish.h"
#include "modes/MenuModeMultiplayerClient.h"
#include "modes/MenuModeMultiplayerServer.h"
#include "modes/MenuModeEditor.h"
#include "modes/MenuModeReplay.h"
#include "modes/MenuModeLoad.h"
#include "modes/GameMode.h"
#include "modes/EditorMode.h"
#include "modes/ConsoleMode.h"
#include "modes/FppMode.h"
#include "utils/MakeUnique.h"


ModeManager::ModeManager(Ogre::RenderWindow* renderWindow, Gui* gui)
  : mInputManager(renderWindow), mGui(gui), mConsoleMode(Utils::make_unique<ConsoleMode>(this)), mIsInConsole(false),
    mRequestedMode(ModeType::NONE), mDiscardActualMode(false)
{
    mInputManager.mKeyboard->setTextTranslation(OIS::Keyboard::Unicode);

    // Loads the main menu
    mApplicationModes.emplace_back(Utils::make_unique<MenuMode>(this));
    mApplicationModes.back()->activate();
}

ModeManager::~ModeManager()
{
}

AbstractApplicationMode* ModeManager::getCurrentMode()
{
    if (mIsInConsole)
        return mConsoleMode.get();

    return mApplicationModes.back().get();
}

ModeManager::ModeType ModeManager::getCurrentModeType()
{
    return getCurrentMode()->getModeType();
}

ModeManager::ModeType ModeManager::getCurrentModeTypeExceptConsole() const
{
    return mApplicationModes.back()->getModeType();
}

void ModeManager::addMode(ModeType mt)
{
    // Check the current mode and return if it was already active.
    if (mApplicationModes.back()->getModeType() == mt)
        return;

    switch(mt)
    {
    // We use a unique console instance.
    case CONSOLE:
        mIsInConsole = true;
        mConsoleMode->activate();
        return;
        break;
    case MENU:
        mApplicationModes.emplace_back(Utils::make_unique<MenuMode>(this));
        break;
    case MENU_SKIRMISH:
        mApplicationModes.emplace_back(Utils::make_unique<MenuModeSkirmish>(this));
        break;
    case MENU_REPLAY:
        mApplicationModes.emplace_back(Utils::make_unique<MenuModeReplay>(this));
        break;
    case MENU_MULTIPLAYER_CLIENT:
        mApplicationModes.emplace_back(Utils::make_unique<MenuModeMultiplayerClient>(this));
        break;
    case MENU_MULTIPLAYER_SERVER:
        mApplicationModes.emplace_back(Utils::make_unique<MenuModeMultiplayerServer>(this));
        break;
    case MENU_EDITOR:
        mApplicationModes.emplace_back(Utils::make_unique<MenuModeEditor>(this));
        break;
    case MENU_CONFIGURE_SEATS:
        mApplicationModes.emplace_back(Utils::make_unique<MenuModeConfigureSeats>(this));
        break;
    case GAME:
        mApplicationModes.emplace_back(Utils::make_unique<GameMode>(this));
        break;
    case EDITOR:
        mApplicationModes.emplace_back(Utils::make_unique<EditorMode>(this));
        break;
    case FPP:
        mApplicationModes.emplace_back(Utils::make_unique<FppMode>(this));
        break;
    case MENU_LOAD_SAVEDGAME:
        mApplicationModes.emplace_back(Utils::make_unique<MenuModeLoad>(this));
        break;
    default:
        break;
    }

    // We're no more in console mode.
    mIsInConsole = false;

    mApplicationModes.back()->activate();
}

void ModeManager::removeMode()
{
    // If we were in the console, we simply switch back
    if (mIsInConsole)
    {
        mIsInConsole = false;
    }
    else if (mApplicationModes.size() > 1)
    {
        mApplicationModes.back()->deactivate();
        mApplicationModes.pop_back();
    }

    mApplicationModes.back()->activate();
}

void ModeManager::checkModeChange()
{
    if (mRequestedMode == NONE)
        return;

    if(mRequestedMode == PREV)
    {
        removeMode();
    }
    else
    {
        mApplicationModes.back()->deactivate();

        if(mDiscardActualMode)
            mApplicationModes.pop_back();

        addMode(mRequestedMode);
    }

    mRequestedMode = NONE;
    mDiscardActualMode = false;
}

void ModeManager::update(const Ogre::FrameEvent& evt)
{
    checkModeChange();

    // We update the current mode
    AbstractApplicationMode* currentMode = getCurrentMode();

    currentMode->getKeyboard()->capture();
    currentMode->getMouse()->capture();

    currentMode->mouseMoved(OIS::MouseEvent(nullptr, currentMode->getMouse()->getMouseState()));

    currentMode->onFrameStarted(evt);
}
