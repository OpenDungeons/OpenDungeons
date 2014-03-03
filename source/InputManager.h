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

#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "ModeManager.h"

#include <OgreVector3.h>
#include <OIS/OISMouse.h>
#include <OIS/OISKeyboard.h>
#include <OIS/OISInputManager.h>

class InputManager
{
public:

    enum DragType
    {
        creature,
        creaturePosses,
        mapLight,
        tileSelection,
        tileBrushSelection,
        addNewRoom,
        addNewTrap,
        rotateAxisX,
        rotateAxisY,
        nullDragType
    };

    InputManager();
    ~InputManager();

    OIS::InputManager*  mInputManager;

    OIS::Keyboard*      mKeyboard;
    bool                mHotkeyLocationIsValid[10];
    Ogre::Vector3       mHotkeyLocation[10];

    //! \brief mouse handling related member
    OIS::Mouse*         mMouse;
    bool                mExpectCreatureClick;
    bool                mLMouseDown, mRMouseDown;
    bool                mMouseDownOnCEGUIWindow;
    bool                mDirectionKeyPressed;

    int                 mXPos, mYPos;
    int                 mLStartDragX, mLStartDragY;
    int                 mRStartDragX, mRStartDragY;
    int                 mDragType;
};

#endif // INPUTMANAGER_H
