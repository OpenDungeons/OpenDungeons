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

#include <boost/noncopyable.hpp>

#include <vector>
#include <string>
#include <memory>

#include "ModeManagerInterface.h"
#include "modes/InputManager.h"

class AbstractApplicationMode;
class ASWrapper;
class InputManager;
class ConsoleMode;
class CameraManager;
class Gui;

namespace Ogre {
  class RenderWindow;
}

class ModeManager : public ModeManagerInterface
{
public:

    using ModePtr_t = std::unique_ptr<AbstractApplicationMode>;

    ModeManager(Ogre::RenderWindow* renderWindow);
    ~ModeManager();

    //! \brief Returns the current mode
    //! Returns the current mode. (If consoleMOde is the current mode, the next mode is returned.)
    AbstractApplicationMode* getCurrentMode();
    ModeType getCurrentModeType();

    //! \brief Returns true if the console is active.
    bool isInConsole()
    {
        return mIsInConsole;
    }

    //! \brief Request loading main menu mode at next update
    void requestMenuMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeType::MENU;
        mDiscardActualMode = discardActualMode;
    }

    //! \brief Request loading level selection menu mode at next update
    void requestMenuSingleplayerMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeType::MENU_SKIRMISH;
        mDiscardActualMode = discardActualMode;
    }

    //! \brief Request loading level selection menu mode at next update
    void requestMenuReplayMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeType::MENU_REPLAY;
        mDiscardActualMode = discardActualMode;
    }

    //! \brief Request Multiplayer menu mode at next update
    void requestMenuMultiplayerClientMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeType::MENU_MULTIPLAYER_CLIENT;
        mDiscardActualMode = discardActualMode;
    }

    //! \brief Request Multiplayer menu mode at next update
    void requestMenuMultiplayerServerMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeType::MENU_MULTIPLAYER_SERVER;
        mDiscardActualMode = discardActualMode;
    }

    //! \brief Request Editor menu mode at next update
    void requestMenuEditorMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeType::MENU_EDITOR;
        mDiscardActualMode = discardActualMode;
    }

    //! \brief Request loading editor mode at next update
    void requestEditorMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeType::EDITOR;
        mDiscardActualMode = discardActualMode;
    }

    //! \brief Request loading console mode at next update
    void requestConsoleMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeType::CONSOLE;
        mDiscardActualMode = discardActualMode;
    }

    //! \brief Request loading FPP mode at next update
    void requestFppMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeType::FPP;
        mDiscardActualMode = discardActualMode;
    }

    //! \brief Request loading game mode at next update
    void requestGameMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeType::GAME;
        mDiscardActualMode = discardActualMode;
     }

    //! \brief Request loading the game seat configuration screen
    void requestConfigureSeatsMode(bool discardActualMode = false)
    {
        mRequestedMode = ModeType::MENU_CONFIGURE_SEATS;
        mDiscardActualMode = discardActualMode;
     }

    //! \brief Request unloading the current mode and activate the parent one
    //! at next update
    void requestUnloadToParentMode()
    {
        mRequestedMode = ModeType::PREV;
    }

    //! \brief Actually change the mode if needed
    void checkModeChange();

    //! \brief Return a reference to the input manager
    InputManager* getInputManager()
    {
        return &mInputManager;
    }

    Gui* getGui()
    {
        return mGui;
    }

private:
    //! \brief The common input manager
    InputManager mInputManager;

    //! \brief GUI reference
    Gui* mGui;

    //! \brief A unique console mode instance, shared between game modes.
    ModePtr_t mConsoleMode;

    //! \brief Tells whether the user is in console mode.
    bool mIsInConsole;

    //! \brief The vector containing the loaded modes.
    //! The active one is either the last one, or the console when
    //! mIsInConsole is equal to true.
    std::vector<ModePtr_t> mApplicationModes;

    //! \brief Tells which new mode is requested.
    ModeType mRequestedMode;

    //! \brief When the new mode will be set, if true, the actual one will be
    //! discarded. That allows to use temporary menu
    bool mDiscardActualMode;

    //! \brief The Angel Script wrapper, used in every game modes
//    ASWrapper* mASWrapper;

    void addMode(ModeType);
    void removeMode();
};

#endif // MODEMANAGER_H
