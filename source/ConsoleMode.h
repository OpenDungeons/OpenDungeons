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
#include "ModeContext.h"

#include <list>
#include <string>

using std::string; using std::list;

class ModeContext;
class PrefixTreeLL;

class  ConsoleMode: public AbstractApplicationMode {

public:

    ConsoleMode(ModeContext*, Console*);

    virtual ~ConsoleMode();

    inline virtual OIS::Mouse* getMouse()
    { return mMc->mMouse; }

    inline virtual OIS::Keyboard* getKeyboard()
    { return mMc->mKeyboard; }

    virtual bool mouseMoved(const OIS::MouseEvent &arg);
    virtual bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
    virtual bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
    virtual bool keyPressed(const OIS::KeyEvent &arg);
    virtual bool keyReleased(const OIS::KeyEvent &arg);
    virtual void handleHotkeys(OIS::KeyCode keycode);
    virtual bool isInGame();
    virtual void giveFocus();

private:
    Console* mConsole;
    PrefixTreeLL* mPrefixTree;
    list<std::string>* mLl;
    std::string mPrefix;
    bool mNonTagKeyPressed;
    std::list<std::string>::iterator mIt;
};

#endif // CONSOLEMODE_H
