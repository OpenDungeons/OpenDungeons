
#ifndef MODECONTEXT_H
#define MODECONTEXT_H
#include <Ogre.h>
#include <OIS.h>
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
    bool                mLMouseDown, mRMouseDown;
    bool                mouseDownOnCEGUIWindow;
    bool                mBrushMode;
    bool                digSetBool;
    bool                directionKeyPressed;
    ModeManager::ModeType            nextMode;
    bool                changed;
    int                 mCurrentFullness, mCurrentTileRadius;
    int                 xPos, yPos;
    int                 mLStartDragX, mLStartDragY;
    int                 mRStartDragX, mRStartDragY;
    int                 mCurrentTileType;
    int                 mDragType;
    std::string         draggedCreature, draggedMapLight;
    GameMap            *gameMap;
    MiniMap            *miniMap;
    };

#endif //MODECONTEXT_H
