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

    // Loads the main menu
    mGameModes.push_back(new MenuMode(this));
    mGameModes.back()->activate();

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
}

ModeManager::~ModeManager()
{
    for (unsigned int i = 0; i < mGameModes.size(); ++i)
        delete mGameModes[i];
    delete mConsoleMode;
    delete mConsole;
    delete mInputManager;
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

void ModeManager::_addGameMode(ModeType mt)
{
    // Check the current mode and return if it was already active.
    if (mGameModes.back()->getModeType() == mt)
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
        mGameModes.push_back(new MenuMode(this));
        break;
    case GAME:
        {
            GameMode* gm = new GameMode(this);
            mGameModes.push_back(gm);
            gm->setLevel(levelToLaunch);
            break;
        }
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

    mGameModes.back()->activate();
}

void ModeManager::_removeGameMode()
{
    // If we were in the console, we simply switch back
    if (mIsInConsole)
    {
        mIsInConsole = false;
    }
    else if (mGameModes.size() > 1)
    {
        delete mGameModes.back();
        mGameModes.pop_back();
    }

    mGameModes.back()->activate();
}

void ModeManager::checkModeChange()
{
    if (mRequestedMode == NONE)
        return;

    if(mRequestedMode == PREV)
        _removeGameMode();
    else
        _addGameMode(mRequestedMode);

    mRequestedMode = NONE;
}
