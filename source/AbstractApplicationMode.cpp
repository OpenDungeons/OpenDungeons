#include "AbstractApplicationMode.h"
#include "ODFrameListener.h"
#include "LogManager.h"
#include "ODApplication.h"


AbstractApplicationMode::AbstractApplicationMode(GameMap* gameMap,MiniMap* mM):
        frameListener(ODFrameListener::getSingletonPtr()),
        mLMouseDown(*(new bool(false))),
        mRMouseDown(*(new bool(false))),
        mBrushMode(*(new bool(false))),
        digSetBool(*(new bool(false))),
        directionKeyPressed(*(new bool(false))),
	mouseDownOnCEGUIWindow(*(new bool(false))),
        mCurrentFullness(*(new int(100))),
        mCurrentTileRadius(*(new int(1))),
        xPos(*(new int(0))),
        yPos(*(new int(0))),
        mLStartDragX(*(new int(0))),
        mLStartDragY(*(new int(0))),
        mRStartDragX(*(new int(0))),
        mRStartDragY(*(new int(0))),
        mCurrentTileType(*(new int(Tile::dirt))),
        mDragType(*(new int(nullDragType))),
	draggedMapLight(*(new std::string(""))),
	draggedCreature(*(new std::string(""))),
        gameMap(gameMap),
	miniMap(mM){
    LogManager::getSingleton().logMessage("*** Initializing OIS ***");

    for (int i = 0; i < 10; ++i)
    {
        hotkeyLocationIsValid[i] = false;
        hotkeyLocation[i] = Ogre::Vector3::ZERO;
    }

    size_t windowHnd = 0;

    ODApplication::getSingleton().getWindow()->getCustomAttribute("WINDOW", &windowHnd);

    std::ostringstream windowHndStr;
    windowHndStr << windowHnd;

    //setup parameter list for OIS
    OIS::ParamList paramList;
    paramList.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));
#if defined OIS_WIN32_PLATFORM
    /* TODO: find out what is the best here (mouse/keyboard exclusiveness to OD or not)
       paramList.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_FOREGROUND" )));
       paramList.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_NONEXCLUSIVE")));
       paramList.insert(std::make_pair(std::string("w32_keyboard"), std::string("DISCL_FOREGROUND")));
       paramList.insert(std::make_pair(std::string("w32_keyboard"), std::string("DISCL_NONEXCLUSIVE")));
    */
#elif defined OIS_LINUX_PLATFORM
    /*
      paramList.insert(std::make_pair(std::string("x11_mouse_grab"), std::string("false")));
      paramList.insert(std::make_pair(std::string("x11_mouse_hide"), std::string("false")));
      paramList.insert(std::make_pair(std::string("x11_keyboard_grab"), std::string("false")));
    */
    paramList.insert(std::make_pair(std::string("XAutoRepeatOn"), std::string("true")));
#endif

    //setup InputManager
    mInputManager = OIS::InputManager::createInputSystem(paramList);

    //setup Keyboard
    mKeyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject(
                                                OIS::OISKeyboard, true));
    mKeyboard->setEventCallback(this);

    //setup Mouse
    mMouse = static_cast<OIS::Mouse*>(mInputManager->createInputObject(OIS::OISMouse, true));
    mMouse->setEventCallback(this);
}
