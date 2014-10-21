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
#include <string>

class AbstractApplicationMode;
class ASWrapper;
class InputManager;
class Console;
class ConsoleMode;
class CameraManager;

namespace Ogre {
  class RenderWindow;
}

class ModeManager
{
    friend class Console;

public:

    enum ModeType
    {
        NONE = 0, // No change requested
        MENU = 1,
        MENU_SKIRMISH,
        MENU_MULTIPLAYER_CLIENT,
        MENU_MULTIPLAYER_SERVER,
        MENU_EDITOR,
        GAME,
        EDITOR,
        CONSOLE,
        FPP,
        PREV // Parent game mode requested
    };

    ModeManager(Ogre::RenderWindow* renderWindow);
    ~ModeManager();

    AbstractApplicationMode* getCurrentMode();
    ModeType getCurrentModeType();

    //! \brief Request loading main menu mode at next update
    void requestMenuMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeManager::MENU;
        mDiscardActualMode = discardActualMode;
    }

    //! \brief Request loading level selection menu mode at next update
    void requestMenuSingleplayerMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeManager::MENU_SKIRMISH;
        mDiscardActualMode = discardActualMode;
    }

    //! \brief Request Multiplayer menu mode at next update
    void requestMenuMultiplayerClientMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeManager::MENU_MULTIPLAYER_CLIENT;
        mDiscardActualMode = discardActualMode;
    }

    //! \brief Request Multiplayer menu mode at next update
    void requestMenuMultiplayerServerMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeManager::MENU_MULTIPLAYER_SERVER;
        mDiscardActualMode = discardActualMode;
    }

    //! \brief Request Editor menu mode at next update
    void requestMenuEditorMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeManager::MENU_EDITOR;
        mDiscardActualMode = discardActualMode;
    }

    //! \brief Request loading editor mode at next update
    void requestEditorMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeManager::EDITOR;
        mDiscardActualMode = discardActualMode;
    }

    //! \brief Request loading console mode at next update
    void requestConsoleMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeManager::CONSOLE;
        mDiscardActualMode = discardActualMode;
    }

    //! \brief Request loading FPP mode at next update
    void requestFppMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeManager::FPP;
        mDiscardActualMode = discardActualMode;
    }

    //! \brief Request loading game mode at next update
    void requestGameMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeType::GAME;
        mDiscardActualMode = discardActualMode;
     }

    //! \brief Request unloading the current mode and activate the parent one
    //! at next update
    void requestUnloadToParentMode()
    {
        mRequestedMode = PREV;
    }

    //! \brief Actually change the mode if needed
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

    //! \brief The vector containing the loaded modes.
    //! The active one is either the last one, or the console when
    //! mIsInConsole is equal to true.
    std::vector<AbstractApplicationMode*> mApplicationModes;

    //! \brief Tells which new mode is requested.
    ModeManager::ModeType mRequestedMode;

    //! \brief When the new mode will be set, if true, the actual one will be
    //! discarded. That allows to use temporary menu
    bool mDiscardActualMode;

    //! \brief The Angel Script wrapper, used in every game modes
    ASWrapper* mASWrapper;

    void addMode(ModeType);
    void removeMode();
};

#endif // MODEMANAGER_H
