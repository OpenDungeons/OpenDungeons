/*!
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

#ifndef ABSTRACTAPPLICATIONMODE_H
#define ABSTRACTAPPLICATIONMODE_H

#include <OIS/OISMouse.h>
#include <OIS/OISKeyboard.h>

#include "Tile.h"
#include "ModeManager.h"
#include "ModeContext.h"

class ODFrameListener;
class GameMap;
class MiniMap;
class Player;

using std::endl; using std::cout;

class AbstractApplicationMode :
    public OIS::MouseListener,
    public OIS::KeyListener
{
protected:
    ModeContext* mc;

public:
    AbstractApplicationMode(ModeContext *modeContext):
        mc(modeContext)
    {};

    virtual ~AbstractApplicationMode() {};

    virtual bool mouseMoved     (const OIS::MouseEvent &arg) = 0;
    virtual bool mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id) = 0;
    virtual bool mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id) = 0;
    virtual bool keyPressed     (const OIS::KeyEvent &arg) = 0;
    virtual bool keyReleased    (const OIS::KeyEvent &arg) = 0;
    virtual void handleHotkeys  (OIS::KeyCode keycode) = 0;

    inline void progressMode (ModeManager::ModeType mm) {
        mc->changed = true;
        mc->nextMode = mm;
    }

    inline void regressMode() {
        mc->changed = true;
        mc->nextMode = ModeManager::PREV;
    }

    virtual OIS::Mouse*      getMouse() = 0;
    virtual OIS::Keyboard*   getKeyboard() = 0;

    enum DragType
    {
        creature,
        mapLight,
        tileSelection,
        tileBrushSelection,
        addNewRoom,
        addNewTrap,
        rotateAxisX,
        rotateAxisY,
        nullDragType
    };

    virtual void giveFocus() = 0;
    virtual bool isInGame() = 0;
};

#endif // ABSTRACTAPPLICATIONMODE_H
