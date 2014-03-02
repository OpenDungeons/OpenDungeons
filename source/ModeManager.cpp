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

#include "ModeContext.h"
#include "MenuMode.h"
#include "GameMode.h"
#include "EditorMode.h"
#include "ConsoleMode.h"
#include "FppMode.h"

ModeManager::ModeManager(GameMap* gameMap, MiniMap* miniMap, Console* console)
{
    mMc = new ModeContext(gameMap, miniMap);

    mGameModes.push_back(new MenuMode(mMc));
    mGameModes.back()->giveFocus();

    // We set a console open in any case.
    mConsole = console;
    mConsoleMode = new ConsoleMode(mMc, console);
    // The console isn't the active one when starting the game
    mIsInConsole = false;
}

ModeManager::~ModeManager()
{
    for (unsigned int i = 0; i < mGameModes.size(); ++i)
        delete mGameModes[i];
    delete mMc;
    delete mConsoleMode;
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
        mGameModes.push_back(new MenuMode(mMc));
        break;
    case GAME:
        mGameModes.push_back(new GameMode(mMc));
        break;
    case EDITOR:
        mGameModes.push_back(new EditorMode(mMc));
        break;
    case FPP:
        mGameModes.push_back(new FppMode(mMc));
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

void ModeManager::lookForNewMode()
{

    if(!mMc->changed)
        return;

    mMc->changed = false;

    if(mMc->nextMode == PREV)
        removeGameMode();
    else
        addGameMode(mMc->nextMode);
}
