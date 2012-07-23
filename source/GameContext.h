#ifndef GAMECONTEXT_H
#define GAMECONTEXT_H
#include <boost/shared_ptr.hpp>

#include <OgreTimer.h>

class GameMap;
class CameraManager;
class ModeManager;
class LogManager;
class RenderManager;




namespace Ogre
{
  class SceneNode;
  class RenderWindow;
}

class GameContext
{
    public:
    GameContext(Ogre::RenderWindow* , ModeManager* ,GameMap*);
        ~GameContext();
        inline GameMap* getGameMap()
        {
            return gameMap;
        }

	void onFrameStarted();
	void onFrameEnded();


    private:
        GameMap* gameMap;
        //TODO: Un-singleton this class and make it a stack object.
        RenderManager*          renderManager;
        CameraManager*          cameraManager;
        ModeManager*            modeManager;
        LogManager*             logManager;
        long int                previousTurn;
        Ogre::SceneNode*        creatureSceneNode;
        Ogre::SceneNode*        roomSceneNode;
        Ogre::SceneNode*        fieldSceneNode;
        Ogre::SceneNode*        lightSceneNode;
        Ogre::Timer             statsDisplayTimer;
        
};

#endif // GAMECONTEXT_H
