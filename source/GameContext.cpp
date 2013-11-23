#include "ASWrapper.h"
#include "ODApplication.h"
#include "CameraManager.h"
#include "LogManager.h"
#include "RenderManager.h"
#include "GameMap.h"
#include "GameContext.h"

GameContext::GameContext(Ogre::RenderWindow* renderWindow, ModeManager* inputManager,GameMap *gm )
    : gameMap(gm),
     cameraManager(NULL)
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
        logManager->logMessage("Creating compositors...", Ogre::LML_NORMAL);
	renderManager->createCompositors();
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
    

    
    logManager->logMessage("Created camera manager");



    gameMap->createTilesMeshes();
    gameMap->hideAllTiles();
    


    // RenderRequest *request = new RenderRequest;
    // request->type = RenderRequest::toggleCreatureVisibility;

    // // Add the request to the queue of rendering operations to be performed before the next frame.
    // RenderManager::queueRenderRequest(request);  
}


void GameContext::onFrameStarted(const Ogre::FrameEvent& evt){

	cameraManager->moveCamera(evt.timeSinceLastFrame);

	gameMap->getMiniMap()->draw();
	gameMap->getMiniMap()->swap();	


}


void GameContext::setCameraManager(CameraManager* tmpCm ){
    cameraManager = tmpCm;



}

void GameContext::onFrameEnded(const Ogre::FrameEvent& evt){


}


GameContext::~GameContext()
{

}
