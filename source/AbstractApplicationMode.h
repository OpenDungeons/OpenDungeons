#ifndef ABSTRACTAPPLICATIONMODE_H
#define ABSTRACTAPPLICATIONMODE_H




/* #include <OIS/OISMouse.h> */

/* class GameStateManager; */
/* namespace Ogre { class FrameEvent; } */
/* namespace OIS { class KeyEvent; } */


/* class AbstractApplicationMode */
/* { */


/*     enum Application { */
/*         MENU, */
/*         GAME, */
/*         EDITOR, */
/* 	FPP, */
/* 	CONSOLE */
/*     }; */

/* public: */
/*     AbstractApplicationMode(GameStateManager* gameStateManager, AbstractApplicationMode* parentState); */
/*     virtual ~AbstractApplicationMode(); */
    
/*     virtual bool frameStarted   (const Ogre::FrameEvent& evt) = 0; */
/*     virtual bool mouseMoved     (const OIS::MouseEvent &arg) = 0; */
/*     virtual bool mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id) = 0; */
/*     virtual bool mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id) = 0; */
/*     virtual bool keyPressed     (const OIS::KeyEvent &arg) = 0; */
/*     virtual bool keyReleased    (const OIS::KeyEvent &arg) = 0; */
/* protected: */
/*     inline GameStateManager* getGameStateManager() */
/*     { */
/*         return gameStateManager; */
/*     } */
/* private: */
/*     GameStateManager* const gameStateManager; */
/*     AbstractApplicationMode* const parentState; */
/* }; */

/*!
 * \file   InputManager.h
 * \date:  02 July 2011
 * \author StefanP.MUC
 * \brief  
 */

/* #ifndef INPUTMANAGER_H_ */
/* #define INPUTMANAGER_H_ */

#include <OIS.h>

#include "Tile.h"




class ODFrameListener;
class GameMap;
class MiniMap;
class Player;

using std::endl; using std::cout;

class AbstractApplicationMode :
        public OIS::MouseListener,
        public OIS::KeyListener
{
    public:
        AbstractApplicationMode(GameMap* ,MiniMap*);
	virtual ~AbstractApplicationMode(){};

        virtual bool mouseMoved     (const OIS::MouseEvent &arg){ cout << "a kuku " << endl;};
        virtual bool mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id){cout << "mouse clicked " << endl;};
        virtual bool mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id){};
        virtual bool keyPressed     (const OIS::KeyEvent &arg){};
        virtual bool keyReleased    (const OIS::KeyEvent &arg){};
        virtual void handleHotkeys  (OIS::KeyCode keycode){};

        inline virtual OIS::Mouse*      getMouse()      const   {return mMouse;}
        inline virtual OIS::Keyboard*   getKeyboard()   const   {return mKeyboard;}

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

        virtual bool isInGame(){}  ;
	

        ODFrameListener*    frameListener;
        OIS::InputManager*  mInputManager;
        OIS::Mouse*         mMouse;
        OIS::Keyboard*      mKeyboard;
        bool                hotkeyLocationIsValid[10];
        Ogre::Vector3       hotkeyLocation[10];
        bool                &mLMouseDown, &mRMouseDown;
        bool                &mouseDownOnCEGUIWindow;
        bool                &mBrushMode;
        bool                &digSetBool;
        bool                &directionKeyPressed;
        int                 &mCurrentFullness, &mCurrentTileRadius;
        int                 &xPos, &yPos;
        int                 &mLStartDragX, &mLStartDragY;
        int                 &mRStartDragX, &mRStartDragY;
        int                 &mCurrentTileType;
        int                 &mDragType;
        std::string         &draggedCreature, &draggedMapLight;
        GameMap            *gameMap;
	MiniMap            *miniMap;
};

/* #endif /\* INPUTMANAGER_H_ *\/ */




#endif // ABSTRACTAPPLICATIONMODE_H
