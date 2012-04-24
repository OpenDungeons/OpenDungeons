#ifndef ABSTRACTGAMESTATE_H
#define ABSTRACTGAMESTATE_H

#include <OIS/OISMouse.h>

class GameStateManager;
namespace Ogre { class FrameEvent; }
namespace OIS { class KeyEvent; }


class AbstractGameState
{

public:
    AbstractGameState(GameStateManager& gameStateManager);
    virtual ~AbstractGameState();
    
    bool frameStarted   (const Ogre::FrameEvent& evt) = 0;
    bool mouseMoved     (const OIS::MouseEvent &arg) = 0;
    bool mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id) = 0;
    bool mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id) = 0;
    bool keyPressed     (const OIS::KeyEvent &arg) = 0;
    bool keyReleased    (const OIS::KeyEvent &arg) = 0;
protected:
    inline GameStateManager& getGameStateManager()
    {
        return gameStateManager;
    }
private:
    GameStateManager& gameStateManager;
    AbstractGameState* parentState;
};

#endif // ABSTRACTGAMESTATE_H
