#include "ModeContext.h"

#include "MapLoader.h"
#include "GameMap.h"
#include "Socket.h"
#include "Network.h"
#include "ClientNotification.h"
#include "ODFrameListener.h"
#include "LogManager.h"
#include "Gui.h"
#include "ODApplication.h"
#include "ResourceManager.h"
#include "TextRenderer.h"
#include "Creature.h"
#include "MapLight.h"
#include "Seat.h"
#include "Trap.h"
#include "Player.h"
#include "RenderManager.h"
#include "CameraManager.h"
#include "Console.h"


ModeContext::ModeContext(GameMap* gameMap,MiniMap * mm):   
        frameListener(ODFrameListener::getSingletonPtr()),
	changed(false),
	mLMouseDown(false),
        mRMouseDown(false),
        directionKeyPressed(false),
        xPos(0),
        yPos(0),
        mLStartDragX(0),
        mLStartDragY(0),
        mRStartDragX(0),
        mRStartDragY(0),
        mDragType(nullDragType),
        gameMap(gameMap),
        expectCreatureClick(false),
	miniMap(mm){
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
/* TODO: find out what is the best here (mouse/keyboard exclusiveness to OD or not) */
    paramList.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_FOREGROUND" )));
    paramList.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_NONEXCLUSIVE")));
    paramList.insert(std::make_pair(std::string("w32_keyboard"), std::string("DISCL_FOREGROUND")));
    paramList.insert(std::make_pair(std::string("w32_keyboard"), std::string("DISCL_NONEXCLUSIVE")));
#elif defined OIS_LINUX_PLATFORM
    paramList.insert(std::make_pair(std::string("x11_mouse_grab"), std::string("false")));
    paramList.insert(std::make_pair(std::string("x11_mouse_hide"), std::string("false")));
    paramList.insert(std::make_pair(std::string("x11_keyboard_grab"), std::string("true")));
    paramList.insert(std::make_pair(std::string("XAutoRepeatOn"), std::string("true")));
#endif 

    //setup InputManager
    mInputManager = OIS::InputManager::createInputSystem(paramList);

    //setup Keyboard
    mKeyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject(
                                                OIS::OISKeyboard, true));
    

    //setup Mouse
    mMouse = static_cast<OIS::Mouse*>(mInputManager->createInputObject(OIS::OISMouse, true));
    

}
