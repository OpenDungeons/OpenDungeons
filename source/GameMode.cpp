/*!
 * \file   InputManager.cpp
 * \date:  02 July 2011
 * \author StefanP.MUC
 * \brief
 */

/* TODO: a lot of the stuff happening in these function should be moved
 * into better places and only be called from here
 * TODO: Make input user-definable
 */

#include "GameMode.h"

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
#include "RenderRequest.h"
#include "RenderManager.h"
#include "CameraManager.h"
#include "Console.h"

#include <algorithm>
#include <vector>
#include <string>

GameMode::GameMode(ModeContext *modeContext):AbstractApplicationMode(modeContext),
					     digSetBool(false)
{


    mMc->mMouse->setEventCallback(this);
    mMc->mKeyboard->setEventCallback(this);

}


GameMode::~GameMode()
{
    LogManager::getSingleton().logMessage("*** Destroying GameMode ***");
    mMc->mInputManager->destroyInputObject(mMc->mMouse);
    mMc->mInputManager->destroyInputObject(mMc->mKeyboard);
    OIS::InputManager::destroyInputSystem(mMc->mInputManager);
    mMc->mInputManager = 0;
}





/*! \brief Process the mouse movement event.
 *
 * The function does a raySceneQuery to determine what object the mouse is over
 * to handle things like dragging out selections of tiles and selecting
 * creatures.
 */
bool GameMode::mouseMoved(const OIS::MouseEvent &arg)
{

    CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition(arg.state.X.abs, arg.state.Y.abs);
    //TODO: do this (and the others isInGame() in here) by GameState

    if (mMc->frameListener->isTerminalActive())
    {

    }
    else
    {
        //If we have a room or trap (or later spell) selected, show what we
        //have selected
        //TODO: This should be changed, or combined with an icon or something later.
        if (mMc->gameMap->getLocalPlayer()->getNewRoomType() || mMc->gameMap->getLocalPlayer()->getNewTrapType())
        {
            TextRenderer::getSingleton().moveText(ODApplication::POINTER_INFO_STRING,
                                                  arg.state.X.abs + 30, arg.state.Y.abs);
        }

        Ogre::RaySceneQueryResult& result = ODFrameListener::getSingleton().doRaySceneQuery(arg);

        Ogre::RaySceneQueryResult::iterator itr = result.begin();
        Ogre::RaySceneQueryResult::iterator end = result.end();
        Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
        std::string resultName = "";
	if(mMc->mDragType == rotateAxisX){
	    mMc->frameListener->cm->move(CameraManager::randomRotateX,arg.state.X.rel);

	}

	else if(mMc->mDragType == rotateAxisY){
	    mMc->frameListener->cm->move(CameraManager::randomRotateY,arg.state.Y.rel);

	}

        else if (mMc->mDragType == tileSelection || mMc->mDragType == addNewRoom || mMc->mDragType == nullDragType)
        {
            // Since this is a tile selection query we loop over the result set and look for the first object which is actually a tile.
            for (; itr != end; ++itr)
            {
                if (itr->movable != NULL)
                {
                    // Check to see if the current query result is a tile.
                    resultName = itr->movable->getName();

                    if (resultName.find("Level_") != std::string::npos)
			{
			    //Make the mouse light follow the mouse
			    //TODO - we should make a pointer to the light or something.
			    Ogre::RaySceneQuery* rq = mMc->frameListener->getRaySceneQuery();
			    Ogre::Real dist = itr->distance;
			    Ogre::Vector3 point = rq->getRay().getPoint(dist);
			    mSceneMgr->getLight("MouseLight")->setPosition(point.x, point.y, 4.0);

			    // Get the x-y coordinates of the tile.
			    sscanf(resultName.c_str(), "Level_%i_%i", &mMc->xPos, &mMc->yPos);
			    RenderRequest *request = new RenderRequest;

			    request->type = RenderRequest::showSquareSelector;
			    request->p = static_cast<void*>(&mMc->xPos);
			    request->p2 = static_cast<void*>(&mMc->yPos);

			    // Add the request to the queue of rendering operations to be performed before the next frame.
			    RenderManager::queueRenderRequest(request);


			    // Make sure the "square selector" mesh is visible and position it over the current tile.

			    //mSceneMgr->getLight("MouseLight")->setPosition(mMc->xPos, mMc->yPos, 2.0);

			    if (mMc->mLMouseDown)
				{
				    // Loop over the tiles in the rectangular selection region and set their setSelected flag accordingly.
				    //TODO: This function is horribly inefficient, it should loop over a rectangle selecting tiles by x-y coords rather than the reverse that it is doing now.
				    std::vector<Tile*> affectedTiles = mMc->gameMap->rectangularRegion(mMc->xPos,
												  mMc->yPos, mMc->mLStartDragX, mMc->mLStartDragY);


				    for (int jj = 0; jj < mMc->gameMap->getMapSizeY(); ++jj)
					{
					    for (int ii = 0; ii < mMc->gameMap->getMapSizeX(); ++ii)
						{
						    mMc->gameMap->getTile(ii,jj)->setSelected(false,mMc->gameMap->getLocalPlayer());
						}
					}

				    if (affectedTiles.size() > 16){

				    }
				    for( std::vector<Tile*>::iterator itr =  affectedTiles.begin(); itr != affectedTiles.end(); ++itr ){

					// for (int jj = 0; jj < GameMap::getMapSizeY(); ++jj)
					//     {
					// 	for (int ii = 0; ii < GameMap::getMapSizeX(); ++ii)
					// 	    {


					// 		for (TileMap_t::iterator itr = mMc->gameMap->firstTile(), last = mMc->gameMap->lastTile();
					// 		        itr != last; ++itr)
					// 		{
					// 		}

					// 		The hell , with the above, I see no reason aditional iteration over tiles in GameMap
					// 		if (std::find(affectedTiles.begin(), affectedTiles.end(), mMc->gameMap->getTile(ii,jj)) != affectedTiles.end())
					// 		    {
					// 			(*itr)->setSelected(true,mMc->gameMap->getLocalPlayer());
					// 		    }
					// 		else
					// 		    {
					// 			(*itr)->setSelected(false,mMc->gameMap->getLocalPlayer());
					// 		    }
					// 	    }
					//     }


					(*itr)->setSelected(true,mMc->gameMap->getLocalPlayer());
				    }
				}

			    if (mMc->mRMouseDown)
				{
				}

			    break;
			}
                }
            }
        }

        else
        {
            // We are dragging a creature but we want to loop over the result set to find the first tile entry,
            // we do this to get the current x-y location of where the "square selector" should be drawn.
            for (; itr != end; ++itr)
            {
                if (itr->movable != NULL)
                {
                    // Check to see if the current query result is a tile.
                    resultName = itr->movable->getName();

                    if (resultName.find("Level_") != std::string::npos)
                    {
                        // Get the x-y coordinates of the tile.
                        sscanf(resultName.c_str(), "Level_%i_%i", &mMc->xPos, &mMc->yPos);


			    RenderRequest *request = new RenderRequest;

			    request->type = RenderRequest::showSquareSelector;
			    request->p = static_cast<void*>(&mMc->xPos);
			    request->p2 = static_cast<void*>(&mMc->yPos);
                        // Make sure the "square selector" mesh is visible and position it over the current tile.

                    }
                }
            }
        }



        if (arg.state.Z.rel > 0)
        {
            if (mMc->mKeyboard->isModifierDown(OIS::Keyboard::Ctrl))
            {
                mMc->gameMap->getLocalPlayer()->rotateCreaturesInHand(1);
            }
            else
            {
                mMc->frameListener->cm->move(CameraManager::moveDown);
            }
        }
        else
            if (arg.state.Z.rel < 0)
            {
                if (mMc->mKeyboard->isModifierDown(OIS::Keyboard::Ctrl))
                {
                    mMc->gameMap->getLocalPlayer()->rotateCreaturesInHand(-1);
                }
                else
                {
                    mMc->frameListener->cm->move(CameraManager::moveUp);
                }
            }
            else
            {
                mMc->frameListener->cm->stopZooming();
            }
    }

    //mMc->frameListener->cm->move(CameraManager::fullStop);

    if (!(mMc->directionKeyPressed || mMc->mDragType == rotateAxisX || mMc->mDragType == rotateAxisY))
    {

	// The fracture value range is done because mouse pointer not always points
	// to the window border , thanks to OIS debugging trick we use to have
	// regrababble window :D

        if (arg.state.X.abs <= 0.01 *  arg.state.width )
            mMc->frameListener->cm->move(CameraManager::moveLeft);
        else
            mMc->frameListener->cm->move(CameraManager::stopLeft);

        if (arg.state.X.abs >= 0.99 *  arg.state.width)
            mMc->frameListener->cm->move(CameraManager::moveRight);
        else
            mMc->frameListener->cm->move(CameraManager::stopRight);

        if (arg.state.Y.abs <= 0.01 *  arg.state.height )
            mMc->frameListener->cm->move(CameraManager::moveForward);
        else
            mMc->frameListener->cm->move(CameraManager::stopForward);

        if (arg.state.Y.abs >= 0.99 * arg.state.height)
            mMc->frameListener->cm->move(CameraManager::moveBackward);
        else
            mMc->frameListener->cm->move(CameraManager::stopBackward);
    }


    //  cerr << arg.state.X.abs <<" " ;
    //  cerr << arg.state.Y.abs <<" " ;
    //  cerr << arg.state.Z.abs <<"\n" ;
    //  cerr << arg.state.width <<"\n" ;
    //  cerr << arg.state.height <<"\n" ;

    //mMc->frameListener->cm->move(CameraManager::moveBackward);


    return true;
}

/*! \brief Handle mouse clicks.
 *
 * This function does a ray scene query to determine what is under the mouse
 * and determines whether a creature or a selection of tiles, is being dragged.
 */
bool GameMode::mousePressed(const OIS::MouseEvent &arg,
                                OIS::MouseButtonID id)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
        Gui::getSingletonPtr()->convertButton(id));

    // If the mouse press is on a CEGUI window ignore it
//    CEGUI::Window *tempWindow =
//        CEGUI::System::getSingleton().getWindowContainingMouse();

    CEGUI::Window *tempWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getWindowContainingMouse();

    if (tempWindow != 0 && tempWindow->getName().compare("Root") != 0)
    {
        mMc->mouseDownOnCEGUIWindow = true;
        return true;
    }
    else
    {
        mMc->mouseDownOnCEGUIWindow = false;
    }

    Ogre::RaySceneQueryResult &result = ODFrameListener::getSingleton().doRaySceneQuery(arg);

    Ogre::RaySceneQueryResult::iterator itr = result.begin();

    std::string resultName;

    // Left mouse button down

    if (id == OIS::MB_Left)
    {
        mMc->mLMouseDown = true;
        mMc->mLStartDragX = mMc->xPos;
        mMc->mLStartDragY = mMc->yPos;

	if(arg.state.Y.abs < 0.1*arg.state.height || arg.state.Y.abs > 0.9*arg.state.height){
		mMc->mDragType=rotateAxisX;
		return true;
	    }

	else if(arg.state.X.abs > 0.9*arg.state.width || arg.state.X.abs < 0.1*arg.state.width){
		mMc->mDragType=rotateAxisY;
		return true;
	    }


        // See if the mouse is over any creatures


        while (itr != result.end())
        {
            if (itr->movable != NULL)
            {
                resultName = itr->movable->getName();

                if (resultName.find("Creature_") != std::string::npos)
                {
                    // if in a game:  Pick the creature up and put it in our hand
		    if(mMc->expectCreatureClick && isInGame() ){

			progressMode(ModeManager::FPP);
			const string& tmp_name =  (itr->movable->getName());
			std::cerr << tmp_name.substr(9, tmp_name.size()) << std::endl;
			mMc->frameListener->cm->setFPPCamera(mMc->gameMap->getCreature(tmp_name.substr( 9, tmp_name.size())));
			mMc->frameListener->cm->setActiveCameraNode("FPP");
			mMc->frameListener->cm->setActiveCamera("FPP");

			mMc->expectCreatureClick =false;
		    }
                    else if (isInGame())
                    {
                        // through away everything before the '_' and then copy the rest into 'array'
                        char array[255];
                        std::stringstream tempSS;
                        tempSS.str(resultName);
                        tempSS.getline(array, sizeof(array), '_');
                        tempSS.getline(array, sizeof(array));

                        Creature *currentCreature = mMc->gameMap->getCreature(array);

                        if (currentCreature != 0 && currentCreature->getColor()
                                == mMc->gameMap->getLocalPlayer()->getSeat()->getColor())
                        {
                            mMc->gameMap->getLocalPlayer()->pickUpCreature(currentCreature);
                            SoundEffectsHelper::getSingleton().playInterfaceSound(
                                SoundEffectsHelper::PICKUP);
                            return true;
                        }
                        else
                        {
                            LogManager::getSingleton().logMessage("Tried to pick up another players creature, or creature was 0");
                        }
                    }
                }

            }

            ++itr;
        }



        // If no creatures or lights are under the  mouse run through the list again to check for tiles
        itr = result.begin();

        while (itr != result.end())
        {
            if (itr->movable != NULL)
            {
                if (resultName.find("Level_") != std::string::npos)
                {
                    // Start by assuming this is a tileSelection drag.
                    mMc->mDragType = tileSelection;

                    // If we are in the map editor, use a brush selection if it has been activated.


                    // If we have selected a room type to add to the map, use a addNewRoom drag type.
                    if (mMc->gameMap->getLocalPlayer()->getNewRoomType() != Room::nullRoomType)
                    {
                        mMc->mDragType = addNewRoom;
                    }

                    // If we have selected a trap type to add to the map, use a addNewTrap drag type.
                    else
                        if (mMc->gameMap->getLocalPlayer()->getNewTrapType() != Trap::nullTrapType)
                        {
                            mMc->mDragType = addNewTrap;
                        }

                    break;
                }
            }

            ++itr;
        }

        // If we are in a game we store the opposite of whether this tile is marked for diggin or not, this allows us to mark tiles
        // by dragging out a selection starting from an unmarcked tile, or unmark them by starting the drag from a marked one.
        if (isInGame())
        {
            Tile *tempTile = mMc->gameMap->getTile(mMc->xPos, mMc->yPos);

            if (tempTile != NULL)
            {
                digSetBool = !(tempTile->getMarkedForDigging(mMc->gameMap->getLocalPlayer()));
            }
        }
    }

    // Right mouse button down
    if (id == OIS::MB_Right)
    {
        mMc->mRMouseDown = true;
        mMc->mRStartDragX = mMc->xPos;
        mMc->mRStartDragY = mMc->yPos;

        // Stop creating rooms, traps, etc.
        mMc->mDragType = nullDragType;
        mMc->gameMap->getLocalPlayer()->setNewRoomType(Room::nullRoomType);
        mMc->gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
        TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "");

        // If we right clicked with the mouse over a valid map tile, try to drop a creature onto the map.
        Tile *curTile = mMc->gameMap->getTile(mMc->xPos, mMc->yPos);

        if (curTile != NULL)
        {
            mMc->gameMap->getLocalPlayer()->dropCreature(curTile);

            if (mMc->gameMap->getLocalPlayer()->numCreaturesInHand() > 0)
            {
                SoundEffectsHelper::getSingleton().playInterfaceSound(SoundEffectsHelper::DROP);
            }
        }
    }

    if (id == OIS::MB_Middle)
    {
        // See if the mouse is over any creatures
        while (itr != result.end())
        {
            if (itr->movable != NULL)
            {
                resultName = itr->movable->getName();

                if (resultName.find("Creature_") != std::string::npos)
                {
                    Creature* tempCreature = mMc->gameMap->getCreature(resultName.substr(
                                                 ((std::string) "Creature_").size(), resultName.size()));

                    if (tempCreature != NULL)
                    {
                        tempCreature->createStatsWindow();
                    }

                    return true;
                }
            }

            ++itr;
        }
    }

    return true;
}

/*! \brief Handle mouse button releases.
 *
 * Finalize the selection of tiles or drop a creature when the user releases the mouse button.
 */
bool GameMode::mouseReleased(const OIS::MouseEvent &arg,
                                 OIS::MouseButtonID id)
{
//    CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->injectMouseButtonUp(
//        Gui::getSingletonPtr()->convertButton(id));

    CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
                Gui::getSingletonPtr()->convertButton(id));

    // If the mouse press was on a CEGUI window ignore it

    if (mMc->mouseDownOnCEGUIWindow)
        return true;

    for (int jj = 0; jj < mMc->gameMap->getMapSizeY(); ++jj)
	{
	    for (int ii = 0; ii < mMc->gameMap->getMapSizeX(); ++ii)
		{
		    mMc->gameMap->getTile(ii,jj)->setSelected(false,mMc->gameMap->getLocalPlayer());
		}
	}


    // Unselect all tiles
    // for (TileMap_t::iterator itr = mMc->gameMap->firstTile(), last = mMc->gameMap->lastTile();
    //         itr != last; ++itr)
    // {
    //     itr->second->setSelected(false,mMc->gameMap->getLocalPlayer());
    // }

    // Left mouse button up
    if (id == OIS::MB_Left)
    {
	if(mMc->mDragType == rotateAxisX){

	    mMc->mDragType=nullDragType;
	}
	else if(mMc->mDragType == rotateAxisY){

	    mMc->mDragType=nullDragType;
	}

        // Check to see if we are moving a creature
        else if (mMc->mDragType == creature)
        {

        }

        // Check to see if we are dragging a map light.
        else
            if (mMc->mDragType == mapLight)
            {

            }

        // Check to see if we are dragging out a selection of tiles or creating a new room
            else
                if (mMc->mDragType == tileSelection || mMc->mDragType == addNewRoom ||
                        mMc->mDragType == addNewTrap)
                {
                    //TODO: move to own function.

                    // Loop over the valid tiles in the affected region.  If we are doing a tileSelection (changing the tile type and fullness) this
                    // loop does that directly.  If, instead, we are doing an addNewRoom, this loop prunes out any tiles from the affectedTiles vector
                    // which cannot have rooms placed on them, then if the player has enough gold, etc to cover the selected tiles with the given room
                    // the next loop will actually create the room.  A similar pruning is done for traps.
                    std::vector<Tile*> affectedTiles = mMc->gameMap->rectangularRegion(mMc->xPos,
                                                       mMc->yPos, mMc->mLStartDragX, mMc->mLStartDragY);
                    std::vector<Tile*>::iterator itr = affectedTiles.begin();

                    while (itr != affectedTiles.end())
                    {
                        Tile *currentTile = *itr;

                        // If we are dragging out tiles.

                        if (mMc->mDragType == tileSelection)
                        {
                            // See if we are in a game or not
                            if (isInGame())
                            {
                                //See if the tile can be marked for digging.
                                if (currentTile->isDiggable())
                                {
                                    if (Socket::serverSocket != NULL)
                                    {
                                        // On the server:  Just mark the tile for digging.
                                        currentTile->setMarkedForDigging(digSetBool,
                                                                         mMc->gameMap->getLocalPlayer());
                                    }
                                    else
                                    {
                                        // On the client:  Inform the server about our choice
                                        ClientNotification *clientNotification =
                                            new ClientNotification;
                                        clientNotification->mType
                                        = ClientNotification::markTile;
                                        clientNotification->mP = currentTile;
                                        clientNotification->mFlag = digSetBool;

                                        sem_wait(&ClientNotification::mClientNotificationQueueLockSemaphore);
                                        ClientNotification::mClientNotificationQueue.push_back(
                                            clientNotification);
                                        sem_post(&ClientNotification::mClientNotificationQueueLockSemaphore);

                                        sem_post(&ClientNotification::mClientNotificationQueueSemaphore);

                                        currentTile->setMarkedForDigging(digSetBool, mMc->gameMap->getLocalPlayer());

                                    }

                                    SoundEffectsHelper::getSingleton().playInterfaceSound(

                                        SoundEffectsHelper::DIGSELECT, false);
                                }
                            }
                        }
                        else // if(mMc->mDragType == ExampleFrameListener::addNewRoom || mMc->mDragType == ExampleFrameListener::addNewTrap)
                        {
                            // If the tile already contains a room, prune it from the list of affected tiles.
                            if (!currentTile->isBuildableUpon())
                            {
                                itr = affectedTiles.erase(itr);
                                continue;
                            }

                            // If we are in a game.
                            if (isInGame())
                            {
                                // If the currentTile is not empty and claimed for my color, then remove it from the affectedTiles vector.
                                if (!(currentTile->getFullness() < 1
                                        && currentTile->getType() == Tile::claimed
                                        && currentTile->colorDouble > 0.99
                                        && currentTile->getColor()
                                        == mMc->gameMap->getLocalPlayer()->getSeat()->color))
                                {
                                    itr = affectedTiles.erase(itr);
                                    continue;
                                }
                            }
                            else // We are in the map editor
                            {
                                // If the currentTile is not empty and claimed, then remove it from the affectedTiles vector.
                                if (!(currentTile->getFullness() < 1
                                        && currentTile->getType() == Tile::claimed))
                                {
                                    itr = affectedTiles.erase(itr);
                                    continue;
                                }
                            }
                        }

                        ++itr;
                    }

                    // If we are adding new rooms the above loop will have pruned out the tiles not eligible
                    // for adding rooms to.  This block then actually adds rooms to the remaining tiles.
                    if (mMc->mDragType == addNewRoom && !affectedTiles.empty())
                    {
                        Room* newRoom = Room::buildRoom(mMc->gameMap, mMc->gameMap->getLocalPlayer()->getNewRoomType(),
                                                        affectedTiles, mMc->gameMap->getLocalPlayer(), !isInGame());

                        if (newRoom == NULL)
                        {
                            //Not enough money
                            //TODO:  play sound or something.
                        }
                    }

                    // If we are adding new traps the above loop will have pruned out the tiles not eligible
                    // for adding traps to.  This block then actually adds traps to the remaining tiles.
                    else
                        if (mMc->mDragType == addNewTrap && !affectedTiles.empty())
                        {
                            Trap* newTrap = Trap::buildTrap(mMc->gameMap, mMc->gameMap->getLocalPlayer()->getNewTrapType(),
                                                            affectedTiles, mMc->gameMap->getLocalPlayer(), !isInGame());

                            if (newTrap == NULL)
                            {
                                //Not enough money
                                //TODO:  play sound or something.
                            }
                        }

                    // Add the tiles which border the affected region to the affectedTiles vector since they may need to have their meshes changed.
                    std::vector<Tile*> borderTiles = mMc->gameMap->tilesBorderedByRegion(
                                                         affectedTiles);

                    affectedTiles.insert(affectedTiles.end(), borderTiles.begin(),
                                         borderTiles.end());

                    // Loop over all the affected tiles and force them to examine their neighbors.  This allows
                    // them to switch to a mesh with fewer polygons if some are hidden by the neighbors, etc.
                    itr = affectedTiles.begin();

                    while (itr != affectedTiles.end())
                    {
                        (*itr)->setFullness((*itr)->getFullness());
                        ++itr;
                    }
                }

        mMc->mLMouseDown = false;
    }

    // Right mouse button up
    if (id == OIS::MB_Right)
    {
        mMc->mRMouseDown = false;
    }

    return true;
}

/*! \brief Handle the keyboard input.
 *
 */
bool GameMode::keyPressed(const OIS::KeyEvent &arg)
{
    //TODO: do this (and the others isInGame() in here) by GameState
    if (mMc->frameListener->isTerminalActive())
    {

    }
    else
    {
        //inject key to Gui
//        CEGUI::System* sys = CEGUI::System::getSingletonPtr();
//        sys->injectKeyDown(arg.key);
//        sys->injectChar(arg.text);

        CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyDown((CEGUI::Key::Scan) arg.key);
        CEGUI::System::getSingleton().getDefaultGUIContext().injectChar(arg.text);


        CameraManager& camMgr = *(mMc->frameListener->cm);

        switch (arg.key)
        {
            case OIS::KC_F11:
                ODFrameListener::getSingleton().toggleDebugInfo();
                break;
            case OIS::KC_GRAVE:
            case OIS::KC_F12:
		progressMode(ModeManager::CONSOLE);
                mMc->frameListener->setTerminalActive(true);
                Console::getSingleton().setVisible(true);
                mMc->mKeyboard->setTextTranslation(OIS::Keyboard::Ascii);
                break;

            case OIS::KC_LEFT:

            case OIS::KC_A:
                mMc->directionKeyPressed = true;

                camMgr.move(camMgr.moveLeft); // Move left
                break;

            case OIS::KC_RIGHT:

            case OIS::KC_D:
                mMc->directionKeyPressed = true;
                camMgr.move(camMgr.moveRight); // Move right
                break;

            case OIS::KC_UP:

            case OIS::KC_W:

                mMc->directionKeyPressed = true;
                camMgr.move(camMgr.moveForward); // Move forward
                break;

            case OIS::KC_DOWN:

            case OIS::KC_S:

                mMc->directionKeyPressed = true;
                camMgr.move(camMgr.moveBackward); // Move backward
                break;

            case OIS::KC_PGUP:

            case OIS::KC_E:
                camMgr.move(camMgr.moveDown); // Move down
                break;

            case OIS::KC_INSERT:

            case OIS::KC_Q:
                camMgr.move(camMgr.moveUp); // Move up
                break;

            case OIS::KC_HOME:
                camMgr.move(camMgr.rotateUp); // Tilt up
                break;

            case OIS::KC_END:
                camMgr.move(camMgr.rotateDown); // Tilt down
                break;

            case OIS::KC_DELETE:
                camMgr.move(camMgr.rotateLeft); // Turn left
                break;

            case OIS::KC_PGDOWN:
                camMgr.move(camMgr.rotateRight); // Turn right
                break;

                //Toggle mMc->mCurrentTileType

            case OIS::KC_R:



                break;

                //Decrease brush radius

            case OIS::KC_COMMA:


                break;

                //Increase brush radius

            case OIS::KC_PERIOD:



                break;

                //Toggle mMc->mBrushMode

            case OIS::KC_B:



                break;

                //Toggle mMc->mCurrentFullness

            case OIS::KC_T:
                // If we are not in a game.


                if(isInGame()) // If we are in a game.
                {
                    Seat* tempSeat = mMc->gameMap->getLocalPlayer()->getSeat();
                    mMc->frameListener->cm->flyTo(Ogre::Vector3(
                                                            tempSeat->startingX, tempSeat->startingY, 0.0));
                }

                break;

                // Quit the game

            case OIS::KC_ESCAPE:
                //MapLoader::writeGameMapToFile(std::string("levels/Test.level") + ".out", *mMc->gameMap);
                //mMc->frameListener->requestExit();
		progressMode(ModeManager::MENU);
                Gui::getSingletonPtr()->switchGuiMode();
                break;

                // Print a screenshot

            case OIS::KC_SYSRQ:
                ResourceManager::getSingleton().takeScreenshot();
                break;

            case OIS::KC_1:

            case OIS::KC_2:

            case OIS::KC_3:

            case OIS::KC_4:

            case OIS::KC_5:

            case OIS::KC_6:

            case OIS::KC_7:

            case OIS::KC_8:

            case OIS::KC_9:

            case OIS::KC_0:
                handleHotkeys(arg.key);
                break;

            default:
                break;
        }
    }

    return true;
}

/*! \brief Process the key up event.
 *
 * When a key is released during normal gamplay the camera movement may need to be stopped.
 */
bool GameMode::keyReleased(const OIS::KeyEvent &arg)
{
    //CEGUI::System::getSingleton().injectKeyUp(arg.key);
    CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyUp((CEGUI::Key::Scan) arg.key);

    if (!mMc->frameListener->isTerminalActive())
    {
        CameraManager& camMgr = *(mMc->frameListener->cm);
        switch (arg.key)
        {

            case OIS::KC_LEFT:

            case OIS::KC_A:
                mMc->directionKeyPressed = false;
                camMgr.move(camMgr.stopLeft);
                break;

            case OIS::KC_RIGHT:

            case OIS::KC_D:
                mMc->directionKeyPressed = false;
                camMgr.move(camMgr.stopRight);
                break;

            case OIS::KC_UP:

            case OIS::KC_W:
                mMc->directionKeyPressed = false;
                camMgr.move(camMgr.stopForward);
                break;

            case OIS::KC_DOWN:

            case OIS::KC_S:
                mMc->directionKeyPressed = false;
                camMgr.move(camMgr.stopBackward);
                break;

            case OIS::KC_PGUP:

            case OIS::KC_E:
                camMgr.move(camMgr.stopDown);
                break;

            case OIS::KC_INSERT:

            case OIS::KC_Q:
                camMgr.move(camMgr.stopUp);
                break;

            case OIS::KC_HOME:
                camMgr.move(camMgr.stopRotUp);
                break;

            case OIS::KC_END:
                camMgr.move(camMgr.stopRotDown);
                break;

            case OIS::KC_DELETE:
                camMgr.move(camMgr.stopRotLeft);
                break;

            case OIS::KC_PGDOWN:
                camMgr.move(camMgr.stopRotRight);
                break;

            default:
                break;
        }
    }

    return true;
}

/*! \brief defines what the hotkeys do
 *
 * currently the only thing the hotkeys do is moving the camera around.
 * If the shift key is pressed we store this hotkey location
 * otherwise we fly the camera to a stored position.
 */
void GameMode::handleHotkeys(OIS::KeyCode keycode)
{
    //keycode minus two because the codes are shifted by two against the actual number
    unsigned int keynumber = keycode - 2;

    if (mMc->mKeyboard->isModifierDown(OIS::Keyboard::Shift))
    {
        mMc->hotkeyLocationIsValid[keynumber] = true;
        mMc->hotkeyLocation[keynumber] = mMc->frameListener->cm->getCameraViewTarget();
    }
    else
    {
        if (mMc->hotkeyLocationIsValid[keynumber])
        {
            mMc->frameListener->cm->flyTo(mMc->hotkeyLocation[keynumber]);
        }
    }
}

/*! \brief Check if we are in editor mode
 *
 */
bool GameMode::isInGame()
{
    //TODO: this exact function is also in ODFrameListener, replace it too after GameState works
    //TODO - we should use a bool or something, not the sockets for this.
    return (Socket::serverSocket != NULL || Socket::clientSocket != NULL);
    //return GameState::getSingletonPtr()->getApplicationState() == GameState::ApplicationState::GAME;
}


void GameMode::giveFocus(){

    mMc->mMouse->setEventCallback(this);
    mMc->mKeyboard->setEventCallback(this);
}


