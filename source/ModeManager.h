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

#ifndef MODEMANAGER_H
#define MODEMANAGER_H

#include <vector>

class AbstractApplicationMode;
class InputManager;
class Console;
class ConsoleMode;
class CameraManager;

class ModeManager
{
    friend class Gui;
    friend class Console;

public:

    enum ModeType
    {
        NONE = 0, // No change requested
        MENU = 1,
        GAME,
        EDITOR,
        CONSOLE,
        FPP,
        PREV // Parent game mode requested
    };

    ModeManager();
    ~ModeManager();

    AbstractApplicationMode* getCurrentMode();
    ModeType getCurrentModeType();

    //! \brief Request loading a new game mode at next update
    void requestNewGameMode(ModeType mm)
    {
        mRequestedMode = mm;
    }

    //! \brief Request unloading the current new game mode and activate the parent one
    //! at next update
    void requestUnloadToParentGameMode()
    {
        mRequestedMode = PREV;
    }

    //! \brief Actually change the game mode if needed
    void checkModeChange();

    InputManager* getInputManager()
    {
        return mInputManager;
    }

private:
    //! \brief The common input manager reference
    InputManager* mInputManager;

    //! \brief A unique console mode instance, shared between game modes.
    ConsoleMode* mConsoleMode;

    //! \brief Tells whether the user is in console mode.
    bool mIsInConsole;

    //! \brief The console instance
    Console* mConsole;

    //! \brief The vector containing the loaded game modes.
    //! The active one is either the last one, or the console when
    //! mIsInConsole is equal to true.
    std::vector<AbstractApplicationMode*> mGameModes;

    //! \brief Tells which new game mode is requested.
    ModeManager::ModeType mRequestedMode;

    AbstractApplicationMode* addGameMode(ModeType);
    AbstractApplicationMode* removeGameMode();
};

#endif // MODEMANAGER_H
