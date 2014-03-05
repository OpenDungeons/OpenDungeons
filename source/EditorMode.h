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

#ifndef EDITORMODE_H
#define EDITORMODE_H

#include "AbstractApplicationMode.h"

#include "GameMap.h"

class Gui; // Used to change the Current tile type

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

    // ! Specific functions
    GameMap* getGameMap()
    {
        return mGameMap;
    }

private:
    bool                mChanged;
    int                 mCurrentFullness;
    int                 mCurrentTileRadius;
    bool                mBrushMode;
    int                 mCurrentTileType;
    DragType            mDragType;
    std::string         mDraggedCreature;
    std::string         mDraggedMapLight;

    //! \brief Rendering members
    GameMap*            mGameMap;

    //! \brief Stores the lastest mouse cursor and light positions.
    int mMouseX;
    int mMouseY;

    //! \brief The Mouse environment light following the mouse, don't delete it.
    Ogre::Light* mMouseLight;

    //! \brief A sub-function called by mouseMoved()
    //! It will handle each drag type and permit easy early inner return
    void handleMouseMovedDragType(const OIS::MouseEvent& arg);

    //! \brief A sub-function called by mouseMoved()
    //! It will handle the potential mouse wheel logic
    void handleMouseWheel(const OIS::MouseEvent& arg);

    //! \brief Handle updating the selector position on screen
    void handleCursorPositionUpdate();
};

#endif // EDITORMODE_H
