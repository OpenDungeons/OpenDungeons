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

#ifndef GAMEMODE_H
#define GAMEMODE_H

#include "AbstractApplicationMode.h"

#include "gamemap/GameMap.h"

#include <CEGUI/EventArgs.h>

namespace CEGUI
{
class Window;
}

class  GameMode: public AbstractApplicationMode
{
 public:
    GameMode(ModeManager*);

    virtual ~GameMode();

    /*! \brief Process the mouse movement event.
     *
     * The function does a raySceneQuery to determine what object the mouse is over
     * to handle things like dragging out selections of tiles and selecting
     * creatures.
     */
    virtual bool mouseMoved     (const OIS::MouseEvent &arg);

    /*! \brief Handle mouse clicks.
     *
     * This function does a ray scene query to determine what is under the mouse
     * and determines whether a creature or a selection of tiles, is being dragged.
     */
    virtual bool mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id);

    /*! \brief Handle mouse button releases.
     *
     * Finalize the selection of tiles or drop a creature when the user releases the mouse button.
     */
    virtual bool mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id);

    //! \brief Handle the keyboard input.
    virtual bool keyPressed     (const OIS::KeyEvent &arg);

    /*! \brief Process the key up event.
     *
     * When a key is released during normal gamplay the camera movement may need to be stopped.
     */
    virtual bool keyReleased    (const OIS::KeyEvent &arg);

    /*! \brief defines what the hotkeys do
     *
     * currently the only thing the hotkeys do is moving the camera around.
     * If the shift key is pressed we store this hotkey location
     * otherwise we fly the camera to a stored position.
     */
    virtual void handleHotkeys  (OIS::KeyCode keycode);

    void onFrameStarted(const Ogre::FrameEvent& evt);
    void onFrameEnded(const Ogre::FrameEvent& evt);

    //! \brief Called when the game mode is activated
    //! Used to call the corresponding Gui Sheet.
    void activate();

    //! \brief Called when exit button is pressed
    void popupExit(bool pause);

    virtual void exitMode();

    virtual void notifyGuiAction(GuiAction guiAction);

 protected:
    //! \brief Handle the keyboard input in normal mode
    virtual bool keyPressedNormal   (const OIS::KeyEvent &arg);

    //! \brief Handle the keyboard input when chat is activated
    virtual bool keyPressedChat     (const OIS::KeyEvent &arg);

    //! \brief Handle the keyboard input in normal mode
    virtual bool keyReleasedNormal  (const OIS::KeyEvent &arg);

    //! \brief Handle the keyboard input when chat is activated
    virtual bool keyReleasedChat    (const OIS::KeyEvent &arg);

private:
    enum InputMode
    {
        InputModeNormal,
        InputModeChat
    };
    //! \brief Sets whether a tile must marked or unmarked for digging.
    //! this value is based on the first marked flag tile selected.
    bool mDigSetBool;

    //! \brief A reference to the game map used by the game mode
    //! For now, handled by the frame listener, don't delete it.
    GameMap* mGameMap;

    //! \brief Stores the lastest mouse cursor and light positions.
    int mMouseX;
    int mMouseY;

    //! \brief The Mouse environment light following the mouse, don't delete it.
    Ogre::Light* mMouseLight;

    InputMode mCurrentInputMode;
    std::vector<OIS::KeyCode> mKeysChatPressed;

    //! \brief A simple window displaying the common game controls.
    //! Useful in the way of a true settings menu.
    CEGUI::Window* mHelpWindow;

    //! \brief Creates the help window.
    void createHelpWindow();

    //! \brief help Hides the help window.
    bool hideHelpWindow(const CEGUI::EventArgs& /*e*/);

    //! \brief Handle updating the selector position on screen
    void handleCursorPositionUpdate();

    //! \brief A sub-function called by mouseMoved()
    //! It will handle the potential mouse wheel logic
    void handleMouseWheel(const OIS::MouseEvent& arg);
};

#endif // GAMEMODE_H
