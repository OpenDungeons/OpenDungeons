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

#ifndef GAMEEDITORMODECONSOLE_H
#define GAMEEDITORMODECONSOLE_H

#include "AbstractApplicationMode.h"

#include "ConsoleInterface.h"

#include <string>

namespace CEGUI
{
class Window;
class EventArgs;
class String;
class Listbox;
class Editbox;
}

class GameEditorModeConsole
{
public:

    GameEditorModeConsole(ModeManager*);

    ~GameEditorModeConsole();

    bool keyPressed(const OIS::KeyEvent &arg);

    //! \brief Called when the game mode is activated
    //! Used to call the corresponding Gui Sheet.
    void activate();

private:
    void printToConsole(const std::string& text);
    bool executeCurrentPrompt(const CEGUI::EventArgs& e = {});
    bool characterEntered(const CEGUI::EventArgs& e = {});

    ConsoleInterface mConsoleInterface;

    CEGUI::Listbox* mConsoleHistoryWindow;
    CEGUI::Editbox* mEditboxWindow;

    ModeManager* mModeManager;

    bool leaveConsole(const CEGUI::EventArgs& e = {});

    inline void addEventConnection(CEGUI::Event::Connection conn)
    {
        mEventConnections.emplace_back(conn);
    }

    // Vector of cegui event bindings to be cleared on exiting the mode
    std::vector<CEGUI::Event::Connection> mEventConnections;
};

#endif // GAMEEDITORMODECONSOLE_H
