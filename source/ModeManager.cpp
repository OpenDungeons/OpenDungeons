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

#include "ModeManager.h"

#include "ASWrapper.h"
#include "InputManager.h"
#include "MenuMode.h"
#include "MenuModeSkirmish.h"
#include "MenuModeMultiplayerClient.h"
#include "MenuModeMultiplayerServer.h"
#include "MenuModeEditor.h"
#include "GameMode.h"
#include "EditorMode.h"
#include "Console.h"
#include "ConsoleMode.h"
#include "FppMode.h"
#include "ODClient.h"
#include "ODServer.h"
#include "RenderManager.h"
#include "ODFrameListener.h"

ModeManager::ModeManager()
{
    mInputManager = new InputManager();
    mInputManager->mKeyboard->setTextTranslation(OIS::Keyboard::Unicode);


    // Loads the main menu
    mApplicationModes.push_back(new MenuMode(this));
    mApplicationModes.back()->activate();

    // NOTE: Console needs to exist BEFORE ASWrapper because it needs it for callback
    // TODO: Merge Console and Console Mode
    Console* console = new Console();

    // We set a console mode loaded in any case.
    mConsole = console;
    mConsoleMode = new ConsoleMode(this, console);
    // The console isn't the active one when starting the game
    mIsInConsole = false;

    // Don't change the application mode for now.
    mRequestedMode = NONE;

    // Init the Angel Script wrapper for game modes
    mASWrapper = new ASWrapper();

    mDiscardActualMode = false;
}

ModeManager::~ModeManager()
{
    for (std::vector<AbstractApplicationMode*>::iterator it = mApplicationModes.begin(); it != mApplicationModes.end(); ++it)
    {
        AbstractApplicationMode* appMode = *it;
        appMode->exitMode();
        delete appMode;
    }
    delete mConsoleMode;
    delete mConsole;
    delete mInputManager;
    delete mASWrapper;
}

AbstractApplicationMode* ModeManager::getCurrentMode()
{
    if (mIsInConsole)
        return mConsoleMode;

    return mApplicationModes.back();
}

ModeManager::ModeType ModeManager::getCurrentModeType()
{
    return getCurrentMode()->getModeType();
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
        mApplicationModes.push_back(new MenuMode(this));
        break;
    case MENU_SKIRMISH:
        mApplicationModes.push_back(new MenuModeSkirmish(this));
        break;
    case MENU_MULTIPLAYER_CLIENT:
        mApplicationModes.push_back(new MenuModeMultiplayerClient(this));
        break;
    case MENU_MULTIPLAYER_SERVER:
        mApplicationModes.push_back(new MenuModeMultiplayerServer(this));
        break;
    case MENU_EDITOR:
        mApplicationModes.push_back(new MenuModeEditor(this));
        break;
    case GAME:
        mApplicationModes.push_back(new GameMode(this));
        break;
    case EDITOR:
        mApplicationModes.push_back(new EditorMode(this));
        break;
    case FPP:
        mApplicationModes.push_back(new FppMode(this));
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
        AbstractApplicationMode* appMode = mApplicationModes.back();
        appMode->exitMode();
        delete appMode;
        mApplicationModes.pop_back();
    }

    mApplicationModes.back()->activate();
}

void ModeManager::checkModeChange()
{
    if (mRequestedMode == NONE)
        return;

    if(mRequestedMode == PREV)
        removeMode();
    else
    {
        if(mDiscardActualMode)
        {
            AbstractApplicationMode* appMode = mApplicationModes.back();
            appMode->exitMode();
            delete appMode;
            mApplicationModes.pop_back();
        }
        addMode(mRequestedMode);
    }

    mRequestedMode = NONE;
    mDiscardActualMode = false;
}
