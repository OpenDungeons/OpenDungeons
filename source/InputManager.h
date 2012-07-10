/*!
 * \file   InputManager.h
 * \date:  02 July 2011
 * \author StefanP.MUC
 * \brief  
 */

#ifndef INPUTMANAGER_H_
#define INPUTMANAGER_H_

#include <OIS.h>

#include "Tile.h"

class ODFrameListener;
class GameMap;
class MiniMap;
class Player;

class InputManager :
        public Ogre::Singleton<InputManager>,
        public OIS::MouseListener,
        public OIS::KeyListener
{
    public:
  InputManager(GameMap* gameMap,MiniMap* miniMap);
        virtual ~InputManager();

        bool mouseMoved     (const OIS::MouseEvent &arg);
        bool mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id);
        bool mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id);
        bool keyPressed     (const OIS::KeyEvent &arg);
        bool keyReleased    (const OIS::KeyEvent &arg);
        void handleHotkeys  (OIS::KeyCode keycode);

        inline OIS::Mouse*      getMouse()      const   {return mMouse;}
        inline OIS::Keyboard*   getKeyboard()   const   {return mKeyboard;}

    private:
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

        InputManager(const InputManager&);

        bool isInGame();

        ODFrameListener*    frameListener;
        OIS::InputManager*  mInputManager;
        OIS::Mouse*         mMouse;
        OIS::Keyboard*      mKeyboard;
        bool                hotkeyLocationIsValid[10];
        Ogre::Vector3       hotkeyLocation[10];
        bool                mLMouseDown, mRMouseDown;
        bool                mouseDownOnCEGUIWindow;
        bool                mBrushMode;
        bool                digSetBool;
        bool                directionKeyPressed;
        int                 mCurrentFullness, mCurrentTileRadius;
        int                 xPos, yPos;
        int                 mLStartDragX, mLStartDragY;
        int                 mRStartDragX, mRStartDragY;
        Tile::TileType      mCurrentTileType;
        DragType            mDragType;
        std::string         draggedCreature, draggedMapLight;
        GameMap*            gameMap;
	MiniMap*            miniMap;
};

#endif /* INPUTMANAGER_H_ */
