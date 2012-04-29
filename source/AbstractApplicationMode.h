#ifndef ABSTRACTAPPLICATIONMODE_H
#define ABSTRACTAPPLICATIONMODE_H

#include <OIS/OISMouse.h>

class GameStateManager;
namespace Ogre { class FrameEvent; }
namespace OIS { class KeyEvent; }


class AbstractApplicationMode
{


    enum ApplicationState {
        MENU,
        GAME,
        EDITOR,
	FPP,
	CONSOLE
    };

public:
    AbstractApplicationMode(GameStateManager* gameStateManager, AbstractApplicationMode* parentState);
    virtual ~AbstractApplicationMode();
    
    virtual bool frameStarted   (const Ogre::FrameEvent& evt) = 0;
    virtual bool mouseMoved     (const OIS::MouseEvent &arg) = 0;
    virtual bool mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id) = 0;
    virtual bool mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id) = 0;
    virtual bool keyPressed     (const OIS::KeyEvent &arg) = 0;
    virtual bool keyReleased    (const OIS::KeyEvent &arg) = 0;
protected:
    inline GameStateManager* getGameStateManager()
    {
        return gameStateManager;
    }
private:
    GameStateManager* const gameStateManager;
    AbstractApplicationMode* const parentState;
};

#endif // ABSTRACTAPPLICATIONMODE_H
