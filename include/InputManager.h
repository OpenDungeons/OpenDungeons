/*!
 * \file   InputManager.cpp
 * \date:  02 July 2011
 * \author StefanP.MUC
 * \brief  
 */

#ifndef INPUTMANAGER_H_
#define INPUTMANAGER_H_

#include <OIS.h>

#include "Tile.h"

class ODFrameListener;
class Tile;

class InputManager :
        public Ogre::Singleton<InputManager>,
        public OIS::MouseListener,
        public OIS::KeyListener
{
    public:
        InputManager(std::string windowHndString);
        virtual ~InputManager();

        static InputManager& getSingleton();
        static InputManager* getSingletonPtr();

        bool mouseMoved(const OIS::MouseEvent &arg);
        bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
        bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
        bool keyPressed(const OIS::KeyEvent &arg);
        bool keyReleased(const OIS::KeyEvent &arg);
        void handleHotkeys(int hotkeyNumber);

        inline OIS::Mouse* getMouse() const{return mMouse;}
        inline OIS::Keyboard* getKeyboard() const{return mKeyboard;}

    private:
        enum DragType
        {
            creature,
            mapLight,
            tileSelection,
            tileBrushSelection,
            addNewRoom,
            addNewTrap,
            nullDragType
        };

        InputManager(const InputManager&);

        bool isInGame();

        ODFrameListener* frameListener;

        OIS::InputManager* mInputManager;
        OIS::Mouse* mMouse;
        OIS::Keyboard* mKeyboard;

        bool hotkeyLocationIsValid[10];
        Ogre::Vector3 hotkeyLocation[10];

        bool mLMouseDown, mRMouseDown; // True if the mouse buttons are down
        bool mouseDownOnCEGUIWindow;
        bool mBrushMode;
        bool digSetBool; // For server mode - hods whether to mark or unmark a tile for digging

        bool mStatsOn; //FIXME: replace ogre stats window with own CEGUI window

        int mCurrentFullness, mCurrentTileRadius;
        int xPos, yPos;
        int mLStartDragX, mLStartDragY; // The start tile coordinates for a left drag
        int mRStartDragX, mRStartDragY; // The start tile coordinates for a left drag

        Tile::TileType mCurrentTileType;

        DragType mDragType;
        std::string draggedCreature, draggedMapLight;
};

#endif /* INPUTMANAGER_H_ */
