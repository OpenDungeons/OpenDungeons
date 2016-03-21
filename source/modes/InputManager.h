/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

namespace OIS
{
    class InputManager;
    class Keyboard;
    class Mouse;
}

namespace Ogre
{
  class RenderWindow;
}

enum class InputCommandState
{
    infoOnly, // When the player is only moving mouse (but not building)
    building, // When the player is building (but has not validated he build)
    validated // When the player is happy with the build and wants to build
};

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
    bool                mLMouseDown, mRMouseDown, mMMouseDown;
    bool                mMouseDownOnCEGUIWindow;

    Ogre::Vector3       mKeeperHandPos;
    int                 mXPos, mYPos;
    int                 mLStartDragX, mLStartDragY;
    int                 mRStartDragX, mRStartDragY;
    //! \brief In editor mode, it contains the selected seat Id. In gamemode, it is not used
    int                 mSeatIdSelected;
    InputCommandState   mCommandState;
};

#endif // INPUTMANAGER_H
