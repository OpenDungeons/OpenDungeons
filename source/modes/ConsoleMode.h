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

#ifndef CONSOLEMODE_H
#define CONSOLEMODE_H

#include "AbstractApplicationMode.h"
#include "ConsoleInterface.h"

#include <list>
#include <string>

namespace CEGUI {
    class Window;
    class EventArgs;
    class String;
}

class  ConsoleMode: public AbstractApplicationMode
{
public:

    ConsoleMode(ModeManager*);

    virtual ~ConsoleMode();

    virtual bool mouseMoved(const OIS::MouseEvent &arg);
    virtual bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
    virtual bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
    virtual bool keyPressed(const OIS::KeyEvent &arg);
    virtual bool keyReleased(const OIS::KeyEvent &arg);
    virtual void handleHotkeys(OIS::KeyCode keycode);

    void onFrameStarted(const Ogre::FrameEvent& evt) {};
    void onFrameEnded(const Ogre::FrameEvent& evt) {};

    //! \brief Called when the game mode is activated
    //! Used to call the corresponding Gui Sheet.
    void activate();

private:
    bool sendButtonClick(const CEGUI::EventArgs& e);
    void executeCommand(const CEGUI::String& command);
    void printToWindow(const std::string& text);

    CEGUI::Window* mConsoleHistoryWindow;
    CEGUI::Window* mEditBoxWindow;

    ConsoleInterface mConsoleInterface;
};

#endif // CONSOLEMODE_H
