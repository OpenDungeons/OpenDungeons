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

#ifndef CONSOLEMODE_H
#define CONSOLEMODE_H

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

class ConsoleMode
{
public:

    ConsoleMode(ModeManager*);

    bool keyPressed(const OIS::KeyEvent &arg);

    //! \brief Called when the game mode is activated
    //! Used to call the corresponding Gui Sheet.
    void activate();

private:
    void printToConsole(const std::string& text);
    bool executeCurrentPrompt(const CEGUI::EventArgs &e);

    ConsoleInterface mConsoleInterface;

    CEGUI::Listbox* mConsoleHistoryWindow;
    CEGUI::Editbox* mEditboxWindow;

    ModeManager* mModeManager;

    bool leaveConsole(const CEGUI::EventArgs& e = {});
};

#endif // CONSOLEMODE_H
