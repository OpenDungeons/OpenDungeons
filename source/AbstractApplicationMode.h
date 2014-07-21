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

#include "ModeManager.h"
#include "InputManager.h"

#include <OIS/OISMouse.h>
#include <OIS/OISKeyboard.h>
#include <OgreFrameListener.h>
#include <OgreWindowEventUtilities.h>

#include <iostream>

using std::endl; using std::cout;

class AbstractApplicationMode :
    public OIS::MouseListener,
    public OIS::KeyListener,
    public Ogre::FrameListener,
    public Ogre::WindowEventListener
{
protected:
    // foreign reference, don't delete it.
    ModeManager* mModeManager;

public:

    AbstractApplicationMode(ModeManager *modeManager, ModeManager::ModeType modeType):
        mModeManager(modeManager),
        mModeType(modeType)
    {};

    virtual ~AbstractApplicationMode()
    {};

    //! \brief Input methods
    enum DragType
    {
        creature,
        mapLight,
        tileSelection,
        addNewRoom,
        addNewTrap,
        nullDragType
    };

    virtual bool mouseMoved     (const OIS::MouseEvent &arg) = 0;
    virtual bool mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id) = 0;
    virtual bool mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id) = 0;
    virtual bool keyPressed     (const OIS::KeyEvent &arg) = 0;
    virtual bool keyReleased    (const OIS::KeyEvent &arg) = 0;
    virtual void handleHotkeys  (OIS::KeyCode keycode) = 0;

    virtual OIS::Mouse* getMouse()
    {
        return mModeManager->getInputManager()->mMouse;
    }

    virtual OIS::Keyboard* getKeyboard()
    {
        return mModeManager->getInputManager()->mKeyboard;
    }

    //! Called when activating the new game mode
    //! Used for instance, to load the corresponding Gui Sheet
    virtual void activate() = 0;

    void regressMode()
    {
        mModeManager->requestUnloadToParentGameMode();
    }

    ModeManager::ModeType getModeType() const
    {
        return mModeType;
    }

    //! \brief Makes the handling of louse and keyboard interact with this mode.
    virtual void giveFocus();

    //! \brief Tells whether the client and the server are speaking to each other.
    //! TODO This should be moved to a better common place such as in the mode manager
    virtual bool isConnected();

    //! \brief Game mode specific rendering methods.
    virtual void onFrameStarted(const Ogre::FrameEvent& evt) = 0;
    virtual void onFrameEnded(const Ogre::FrameEvent& evt) = 0;

protected:
    // The game mode type;
    ModeManager::ModeType mModeType;
};

#endif // ABSTRACTAPPLICATIONMODE_H
