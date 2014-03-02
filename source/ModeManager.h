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
class ModeContext;
class GameMap;
class MiniMap;
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
        MENU,
        GAME,
        EDITOR,
        CONSOLE,
        FPP,
        PREV
    };

    ModeManager(GameMap*, MiniMap*, Console*);
    ~ModeManager();

    AbstractApplicationMode* getCurrentMode();

    ModeType getCurrentModeType();

    AbstractApplicationMode* addGameMode(ModeType);
    AbstractApplicationMode* removeGameMode();

    void lookForNewMode();

private:
    // \brief The mode context handler
    ModeContext* mMc;

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
};

#endif // MODEMANAGER_H
