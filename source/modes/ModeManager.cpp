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
#include "utils/MakeUnique.h"


ModeManager::ModeManager(Ogre::RenderWindow* renderWindow, Gui* gui) :
    mInputManager(renderWindow),
    mGui(gui),
    mRequestedMode(ModeType::NONE)
{
    mInputManager.mKeyboard->setTextTranslation(OIS::Keyboard::Unicode);

    // Loads the main menu
    mCurrentApplicationMode = Utils::make_unique<MenuMode>(this);
    mCurrentApplicationMode->activate();
}

ModeManager::~ModeManager()
{
}

AbstractApplicationMode* ModeManager::getCurrentMode()
{
    return mCurrentApplicationMode.get();
}

ModeManager::ModeType ModeManager::getCurrentModeType() const
{
    return mCurrentApplicationMode->getModeType();
}

void ModeManager::requestMode(ModeType mode)
{
    mPreviousModeTypes.push_back(mCurrentApplicationMode->getModeType());
    mRequestedMode = mode;
}

void ModeManager::requestPreviousMode()
{
    if (mPreviousModeTypes.empty())
    {
        mRequestedMode = MENU_MAIN;
        return;
    }

    mRequestedMode = mPreviousModeTypes.back();
    mPreviousModeTypes.pop_back();
}

void ModeManager::checkModeChange()
{
    if (mRequestedMode == NONE)
        return;

    if (mCurrentApplicationMode->getModeType() == mRequestedMode)
        return;

    mCurrentApplicationMode->deactivate();
    mCurrentApplicationMode = nullptr;

    switch(mRequestedMode)
    {
    case MENU_MAIN:
        mCurrentApplicationMode = Utils::make_unique<MenuMode>(this);
        // In that case, we are at the root menu and should ensure to have
        // a clean mode type history
        mPreviousModeTypes.clear();
        break;
    case MENU_SKIRMISH:
        mCurrentApplicationMode = Utils::make_unique<MenuModeSkirmish>(this);
        break;
    case MENU_REPLAY:
        mCurrentApplicationMode = Utils::make_unique<MenuModeReplay>(this);
        break;
    case MENU_MULTIPLAYER_CLIENT:
        mCurrentApplicationMode = Utils::make_unique<MenuModeMultiplayerClient>(this);
        break;
    case MENU_MULTIPLAYER_SERVER:
        mCurrentApplicationMode = Utils::make_unique<MenuModeMultiplayerServer>(this);
        break;
    case MENU_EDITOR:
        mCurrentApplicationMode = Utils::make_unique<MenuModeEditor>(this);
        break;
    case MENU_CONFIGURE_SEATS:
        mCurrentApplicationMode = Utils::make_unique<MenuModeConfigureSeats>(this);
        break;
    case GAME:
        mCurrentApplicationMode = Utils::make_unique<GameMode>(this);
        break;
    case EDITOR:
        mCurrentApplicationMode = Utils::make_unique<EditorMode>(this);
        break;
    case MENU_LOAD_SAVEDGAME:
        mCurrentApplicationMode = Utils::make_unique<MenuModeLoad>(this);
        break;
    default:
        break;
    }

    mCurrentApplicationMode->activate();

    mRequestedMode = NONE;
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
