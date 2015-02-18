/*!
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

#ifndef ABSTRACTAPPLICATIONMODE_H
#define ABSTRACTAPPLICATIONMODE_H

#include "modes/ModeManager.h"
#include "modes/InputManager.h"

#include <OIS/OISMouse.h>
#include <OIS/OISKeyboard.h>

#include <CEGUI/EventArgs.h>
#include <CEGUI/Event.h>

#include <iostream>
#include <vector>

class GameEntity;

class AbstractApplicationMode :
    public OIS::MouseListener,
    public OIS::KeyListener
{
public:
    AbstractApplicationMode(ModeManager *modeManager, ModeManager::ModeType modeType):
        mModeManager(modeManager),
        mModeType(modeType)
    {}

    virtual ~AbstractApplicationMode() override;

    //! \brief Input methods
    enum DragType
    {
        tileSelection,
        addNewRoom,
        addNewTrap,
        changeTile,
        nullDragType
    };

    enum GuiAction
    {
        ButtonPressedCreatureWorker,
        ButtonPressedCreatureFighter,
        ButtonPressedMapLight
    };

    virtual void notifyGuiAction(GuiAction guiAction)
    { }

    virtual bool mouseMoved     (const OIS::MouseEvent &arg) override;
    virtual bool mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id) override;
    virtual bool mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id) override;
    virtual bool keyPressed     (const OIS::KeyEvent &arg) override;
    virtual bool keyReleased    (const OIS::KeyEvent &arg) override;
    virtual void handleHotkeys  (OIS::KeyCode keycode) {};

    virtual OIS::Mouse* getMouse()
    {
        return mModeManager->getInputManager()->mMouse;
    }

    virtual OIS::Keyboard* getKeyboard()
    {
        return mModeManager->getInputManager()->mKeyboard;
    }

    //! Called when activating a new mode
    //! Used for example, to load the corresponding Gui Sheet
    virtual void activate() = 0;

    void regressMode()
    {
        mModeManager->requestUnloadToParentMode();
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
    virtual void onFrameStarted(const Ogre::FrameEvent& evt) {};
    virtual void onFrameEnded(const Ogre::FrameEvent& evt) {};

    virtual void exitMode() {}

    GameEntity* getEntityFromOgreName(const std::string& entityName);

protected:
    ModeManager& getModeManager()
    {
        return *mModeManager;
    }

    void subscribeCloseButton(CEGUI::Window& rootWindow);

    //! \brief Utility function that calls regressMode, used for events
    inline bool regressModeEvent(const CEGUI::EventArgs&)
    {
        regressMode();
        return true;
    }

    inline void addEventConnection(CEGUI::Event::Connection conn)
    {
        mEventConnections.emplace_back(conn);
    }

    //! \brief Returns true if the key is to be processed by the chat.
    //! False otherwise. If false is returned, the key will be processed
    //! by the normal function even if in chat mode
    virtual bool isChatKey          (const OIS::KeyEvent &arg);
    virtual int getChatChar         (const OIS::KeyEvent &arg);

    // foreign reference, don't delete it.
    ModeManager* mModeManager;

    // The game mode type;
    ModeManager::ModeType mModeType;

    // Vector of cegui event bindings to be cleared on exiting the mode
    std::vector<CEGUI::Event::Connection> mEventConnections;
};

#endif // ABSTRACTAPPLICATIONMODE_H
