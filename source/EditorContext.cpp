#include "ASWrapper.h"
#include "ODApplication.h"
#include "CameraManager.h"
#include "LogManager.h"
#include "RenderManager.h"
#include "GameMap.h"

#include "EditorContext.h"

EditorContext::EditorContext(Ogre::RenderWindow* renderWindow, ModeManager* inputManager,GameMap *gm )
    : gameMap(gm)
{


    logManager = LogManager::getSingletonPtr();
    renderManager = RenderManager::getSingletonPtr();
    //gameMap = new GameMap;
    renderManager->setGameMap(gameMap);
    

    //NOTE This is moved here temporarily.
    try
    {
        logManager->logMessage("Creating camera...", Ogre::LML_NORMAL);
        renderManager->createCamera();
        logManager->logMessage("Creating viewports...", Ogre::LML_NORMAL);
        renderManager->createViewports();
        logManager->logMessage("Creating scene...", Ogre::LML_NORMAL);
        renderManager->createScene();
    }
    catch(Ogre::Exception& e)
    {
        ODApplication::displayErrorMessage("Ogre exception when ininialising the render manager:\n"
            + e.getFullDescription(), false);
        exit(0);
        //cleanUp();
        //return;
    }
    catch (std::exception& e)
    {
        ODApplication::displayErrorMessage("Exception when ininialising the render manager:\n"
            + std::string(e.what()), false);
        exit(0);
        //cleanUp();
        //return;
    }
    
    cameraManager = new CameraManager(renderManager->getCamera(),gameMap);
    cameraManager->setModeManager(inputManager);
    
    logManager->logMessage("Created camera manager");


    new ASWrapper();

    gameMap->createTilesMeshes();
    gameMap->hideAllTiles();

}


void EditorContext::onFrameStarted(const Ogre::FrameEvent& evt){

    CameraManager::getSingleton().moveCamera(evt.timeSinceLastFrame);

    gameMap->getMiniMap()->draw();
    gameMap->getMiniMap()->swap();	
    cameraManager->onFrameStarted();

}







void EditorContext::onFrameEnded(const Ogre::FrameEvent& evt){

}


EditorContext::~EditorContext()
{

}
