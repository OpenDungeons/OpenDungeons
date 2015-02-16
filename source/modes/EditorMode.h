/*
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

#ifndef EDITORMODE_H
#define EDITORMODE_H

#include "AbstractApplicationMode.h"

#include "gamemap/MiniMap.h"

class Gui; // Used to change the Current tile type
class GameMap;

enum class TileType;

class  EditorMode: public AbstractApplicationMode
{
    friend class Gui;
public:
    EditorMode(ModeManager* modeManager);

    virtual ~EditorMode();

    virtual bool mouseMoved     (const OIS::MouseEvent &arg);
    virtual bool mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id);
    virtual bool mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id);
    virtual bool keyPressed     (const OIS::KeyEvent &arg);
    virtual bool keyReleased    (const OIS::KeyEvent &arg);
    virtual void handleHotkeys  (OIS::KeyCode keycode);

    void onFrameStarted(const Ogre::FrameEvent& evt);
    void onFrameEnded(const Ogre::FrameEvent& evt);

    //! \brief Called when the game mode is activated
    //! Used to call the corresponding Gui Sheet.
    void activate();

    // ! Specific functions
    GameMap* getGameMap()
    {
        return mGameMap;
    }

    virtual void exitMode();

    virtual void notifyGuiAction(GuiAction guiAction);

private:
    //! \brief Tile type (Dirt, Lava, ...)
    TileType mCurrentTileType;

    //! \brief how of the wall type is there (0 - 100.0)
    //! < 1.0 means no walls.
    double mCurrentFullness;

    //! \brief Current selected seat id
    int mCurrentSeatId;

    //! \brief Current selected creature to spawn
    uint32_t mCurrentCreatureIndex;

    //! \brief The creature node name being dragged by the mouse
    std::string mDraggedCreature;

    //! \brief The map light node name being dragged by the mouse
    std::string mDraggedMapLight;

    //! \brief Rendering members
    GameMap* mGameMap;

    //! \brief The minimap used in this mode
    MiniMap mMiniMap;

    //! \brief Stores the lastest mouse cursor and light positions.
    int mMouseX;
    int mMouseY;

    //! \brief A sub-function called by mouseMoved()
    //! It will handle each drag type and permit easy early inner return
    void handleMouseMovedDragType(const OIS::MouseEvent& arg);

    //! \brief A sub-function called by mouseMoved()
    //! It will handle the potential mouse wheel logic
    void handleMouseWheel(const OIS::MouseEvent& arg);

    //! \brief Handle updating the selector position on screen
    void handleCursorPositionUpdate();

    //! \brief Updates the text seen next to the cursor position.
    //! This text gives the tile position, and the current left-click action
    void updateCursorText();

    //! \brief Minimap click event handler (currently duplicated in GameMode)
    bool onMinimapClick(const CEGUI::EventArgs& arg);
};

#endif // EDITORMODE_H
