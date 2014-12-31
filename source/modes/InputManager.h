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

#include <OgreVector3.h>
#include <OIS/OISMouse.h>
#include <OIS/OISKeyboard.h>
#include <OIS/OISInputManager.h>

namespace Ogre {
  class RenderWindow;
}

class InputManager
{
public:
    InputManager(Ogre::RenderWindow* renderWindow);
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

    int                 mXPos, mYPos;
    int                 mLStartDragX, mLStartDragY;
    int                 mRStartDragX, mRStartDragY;
};

#endif // INPUTMANAGER_H
