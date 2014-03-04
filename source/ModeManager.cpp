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
#include "GameMode.h"
#include "EditorMode.h"
#include "Console.h"
#include "ConsoleMode.h"
#include "FppMode.h"

ModeManager::ModeManager()
{
    mInputManager = new InputManager();

    mGameModes.push_back(new MenuMode(this));
    mGameModes.back()->giveFocus();

    // NOTE: Console needs to exist BEFORE ASWrapper because it needs it for callback
    // TODO: Merge Console and Console Mode
    Console* console = new Console(this);

    // We set a console mode loaded in any case.
    mConsole = console;
    mConsoleMode = new ConsoleMode(this, console);
    // The console isn't the active one when starting the game
    mIsInConsole = false;

    // Don't change the application mode for now.
    mRequestedMode = NONE;

    // Init the Angel Script wrapper for game modes
    mASWrapper = new ASWrapper();
}

ModeManager::~ModeManager()
{
    for (unsigned int i = 0; i < mGameModes.size(); ++i)
        delete mGameModes[i];
    delete mInputManager;
    delete mConsoleMode;
    delete mConsole;
    delete mASWrapper;
}

AbstractApplicationMode* ModeManager::getCurrentMode()
{
    if (mIsInConsole)
        return mConsoleMode;

    return mGameModes.back();
}

ModeManager::ModeType ModeManager::getCurrentModeType()
{
    return getCurrentMode()->getModeType();
}

AbstractApplicationMode* ModeManager::addGameMode(ModeType mm)
{
    // Check the current mode and return if it was already active.
    if (mGameModes.back()->getModeType() == mm)
        return mGameModes.back();

    switch(mm)
    {
    // We use a unique console instance.
    case CONSOLE:
        mIsInConsole = true;
        mConsoleMode->giveFocus();
        return mConsoleMode;
        break;
    case MENU:
        mGameModes.push_back(new MenuMode(this));
        break;
    case GAME:
        mGameModes.push_back(new GameMode(this));
        break;
    case EDITOR:
        mGameModes.push_back(new EditorMode(this));
        break;
    case FPP:
        mGameModes.push_back(new FppMode(this));
        break;
    default:
        break;
    }

    // We're no more in console mode.
    mIsInConsole = false;

    mGameModes.back()->giveFocus();
    return mGameModes.back();
}

AbstractApplicationMode* ModeManager::removeGameMode()
{
    // If we were in the console, we simply switch back
    if (mIsInConsole)
    {
        mIsInConsole = false;
        return mGameModes.back();
    }

    if(mGameModes.size() > 1)
    {
        delete mGameModes.back();
        mGameModes.pop_back();
        mGameModes.back()->giveFocus();
        return mGameModes.back();
    }
    else // We never remove the main menu
        return mGameModes.back();
}

void ModeManager::checkModeChange()
{
    if (mRequestedMode == NONE)
        return;

    if(mRequestedMode == PREV)
        removeGameMode();
    else
        addGameMode(mRequestedMode);

    mRequestedMode = NONE;
}
