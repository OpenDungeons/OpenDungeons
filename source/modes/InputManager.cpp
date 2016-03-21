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

#include "InputManager.h"

#include "utils/ConfigManager.h"
#include "utils/LogManager.h"

#include <OIS/OISMouse.h>
#include <OIS/OISKeyboard.h>
#include <OIS/OISInputManager.h>
#include <OgreRenderWindow.h>

InputManager::InputManager(Ogre::RenderWindow* renderWindow):
    mInputManager(nullptr),
    mKeyboard(nullptr),
    mMouse(nullptr),
    mLMouseDown(false),
    mRMouseDown(false),
    mMMouseDown(false),
    mMouseDownOnCEGUIWindow(false),
    mKeeperHandPos(Ogre::Vector3::ZERO),
    mXPos(0),
    mYPos(0),
    mLStartDragX(0),
    mLStartDragY(0),
    mRStartDragX(0),
    mRStartDragY(0),
    mSeatIdSelected(0),
    mCommandState(InputCommandState::infoOnly)
{
    OD_LOG_INF("*** Initializing OIS - Input Manager ***");

    for (int i = 0; i < 10; ++i)
    {
        mHotkeyLocationIsValid[i] = false;
        mHotkeyLocation[i] = Ogre::Vector3::ZERO;
    }

    // Get the Window attribute for OIS.
    size_t windowHnd = 0;
    renderWindow->getCustomAttribute("WINDOW", &windowHnd);
    std::ostringstream windowHndStr;
    windowHndStr << windowHnd;

    ConfigManager& config = ConfigManager::getSingleton();
    bool mouseGrab = config.getInputValue(Config::MOUSE_GRAB, "No", false) == "Yes";
    bool keyboardGrab = config.getInputValue(Config::KEYBOARD_GRAB, "No", false) == "Yes";

    //setup parameter list for OIS
    OIS::ParamList paramList;
    paramList.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));
#if defined OIS_WIN32_PLATFORM
    paramList.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_FOREGROUND" )));
    paramList.insert(std::make_pair(std::string("w32_mouse"), std::string(mouseGrab ? "DISCL_EXCLUSIVE" : "DISCL_NONEXCLUSIVE")));
    paramList.insert(std::make_pair(std::string("w32_keyboard"), std::string("DISCL_FOREGROUND")));
    paramList.insert(std::make_pair(std::string("w32_keyboard"), std::string(keyboardGrab ? "DISCL_EXCLUSIVE" : "DISCL_NONEXCLUSIVE")));
#elif defined OIS_LINUX_PLATFORM
    paramList.insert(std::make_pair(std::string("x11_mouse_grab"), std::string(mouseGrab ? "true" : "false")));
    paramList.insert(std::make_pair(std::string("x11_mouse_hide"), std::string("false")));
    paramList.insert(std::make_pair(std::string("x11_keyboard_grab"), std::string(keyboardGrab ? "true" : "false")));
    paramList.insert(std::make_pair(std::string("XAutoRepeatOn"), std::string("true")));
#endif

    //setup InputManager
    mInputManager = OIS::InputManager::createInputSystem(paramList);

    //setup Keyboard
    mKeyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject(OIS::OISKeyboard, true));

    //setup Mouse
    mMouse = static_cast<OIS::Mouse*>(mInputManager->createInputObject(OIS::OISMouse, true));
}

InputManager::~InputManager()
{
    OD_LOG_INF("*** Destroying Input Manager ***");
    mInputManager->destroyInputObject(mMouse);
    mInputManager->destroyInputObject(mKeyboard);
    OIS::InputManager::destroyInputSystem(mInputManager);
    mInputManager = nullptr;
}
