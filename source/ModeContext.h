
#ifndef MODECONTEXT_H
#define MODECONTEXT_H
#include <OgreVector3.h>
#include <OIS/OISMouse.h>
#include <OIS/OISKeyboard.h>
#include <OIS/OISInputManager.h>

#include <string>
#include "ModeManager.h"

class GameMap;
class MiniMap;
class ODFrameListener;

struct ModeContext{

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


    ModeContext(GameMap* gm, MiniMap* mm);
    ODFrameListener*    frameListener;
    OIS::InputManager*  mInputManager;
    OIS::Mouse*         mMouse;
    OIS::Keyboard*      mKeyboard;
    bool                (hotkeyLocationIsValid) [10];
    Ogre::Vector3       (hotkeyLocation)  [10];
    bool                expectCreatureClick;
    bool                mLMouseDown, mRMouseDown;
    bool                mouseDownOnCEGUIWindow;
    bool                directionKeyPressed;
    bool                changed;
    ModeManager::ModeType            nextMode;
    int                 xPos, yPos;
    int                 mLStartDragX, mLStartDragY;
    int                 mRStartDragX, mRStartDragY;
    int                 mDragType;
    GameMap            *gameMap;
    MiniMap            *miniMap;
    };

#endif //MODECONTEXT_H
