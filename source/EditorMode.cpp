

#include <algorithm>
#include <vector>
#include <string>

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
#include <OgreEntity.h>

#include "EditorMode.h"


EditorMode::EditorMode(ModeContext *modeContext):AbstractApplicationMode(modeContext),        
						 mCurrentTileType(Tile::dirt),
						 mBrushMode(false),
						 mCurrentFullness(100),
						 mCurrentTileRadius(1)

{



}

EditorMode::~EditorMode(){



}


bool EditorMode::mouseMoved(const OIS::MouseEvent &arg){

    CEGUI::System::getSingleton().injectMousePosition(arg.state.X.abs, arg.state.Y.abs);
    //TODO: do this (and the others isInGame() in here) by GameState

    if (mc->frameListener->isTerminalActive())
    {

    }
    else
    {
        //If we have a room or trap (or later spell) selected, show what we
        //have selected
        //TODO: This should be changed, or combined with an icon or something later.
        if (mc->gameMap->getLocalPlayer()->getNewRoomType() || mc->gameMap->getLocalPlayer()->getNewTrapType())
        {
            TextRenderer::getSingleton().moveText(ODApplication::POINTER_INFO_STRING,
                                                  arg.state.X.abs + 30, arg.state.Y.abs);
        }

        Ogre::RaySceneQueryResult& result = ODFrameListener::getSingleton().doRaySceneQuery(arg);

        Ogre::RaySceneQueryResult::iterator itr = result.begin();
        Ogre::RaySceneQueryResult::iterator end = result.end();
        Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
        std::string resultName = "";
	if(mc->mDragType == rotateAxisX){
	    CameraManager::getSingleton().move(CameraManager::randomRotateX, arg.state.X.rel);

	}

	else if(mc->mDragType == rotateAxisY){
	    CameraManager::getSingleton().move(CameraManager::randomRotateY, arg.state.Y.rel);

	}

        else if (mc->mDragType == tileSelection || mc->mDragType == addNewRoom || mc->mDragType == nullDragType)
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
			    Ogre::RaySceneQuery* rq = mc->frameListener->getRaySceneQuery();
			    Ogre::Real dist = itr->distance;
			    Ogre::Vector3 point = rq->getRay().getPoint(dist);
			    mSceneMgr->getLight("MouseLight")->setPosition(point.x, point.y, 4.0);

			    // Get the x-y coordinates of the tile.

			    //warning obsolete cstdio function
			    sscanf(resultName.c_str(), "Level_%i_%i", &mc->xPos, &mc->yPos);
			    RenderRequest *request = new RenderRequest;

			    request->type = RenderRequest::showSquareSelector;
			    request->p = static_cast<void*>(&mc->xPos);
			    request->p2 = static_cast<void*>(&mc->yPos);

			    // Add the request to the queue of rendering operations to be performed before the next frame.
			    RenderManager::queueRenderRequest(request);
			    
			    
			    // Make sure the "square selector" mesh is visible and position it over the current tile.

			    //mSceneMgr->getLight("MouseLight")->setPosition(mc->xPos, mc->yPos, 2.0);

			    if (mc->mLMouseDown)
				{
				    // Loop over the tiles in the rectangular selection region and set their setSelected flag accordingly.
				    //TODO: This function is horribly inefficient, it should loop over a rectangle selecting tiles by x-y coords rather than the reverse that it is doing now.
				    std::vector<Tile*> affectedTiles = mc->gameMap->rectangularRegion(mc->xPos,
												  mc->yPos, mc->mLStartDragX, mc->mLStartDragY);


				    for (int jj = 0; jj < mc->gameMap->mapSizeY; ++jj)
					{
					    for (int ii = 0; ii < mc->gameMap->mapSizeX; ++ii)
						{
						    mc->gameMap->getTile(ii,jj)->setSelected(false,mc->gameMap->getLocalPlayer());
						}
					}


				    for( std::vector<Tile*>::iterator itr =  affectedTiles.begin(); itr != affectedTiles.end(); ++itr ){

					// for (int jj = 0; jj < GameMap::mapSizeY; ++jj)
					//     {
					// 	for (int ii = 0; ii < GameMap::mapSizeX; ++ii)
					// 	    {


					// 		for (TileMap_t::iterator itr = mc->gameMap->firstTile(), last = mc->gameMap->lastTile();
					// 		        itr != last; ++itr)
					// 		{
					// 		}

					// 		The hell , with the above, I see no reason aditional iteration over tiles in GameMap
					// 		if (std::find(affectedTiles.begin(), affectedTiles.end(), mc->gameMap->getTile(ii,jj)) != affectedTiles.end())
					// 		    {
					// 			(*itr)->setSelected(true,mc->gameMap->getLocalPlayer());
					// 		    }
					// 		else
					// 		    {
					// 			(*itr)->setSelected(false,mc->gameMap->getLocalPlayer());
					// 		    }
					// 	    }
					//     }


					(*itr)->setSelected(true,mc->gameMap->getLocalPlayer());				  
				    }
				}

			    if (mc->mRMouseDown)
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
                        sscanf(resultName.c_str(), "Level_%i_%i", &mc->xPos, &mc->yPos);


			    RenderRequest *request = new RenderRequest;

			    request->type = RenderRequest::showSquareSelector;
			    request->p = static_cast<void*>(&mc->xPos);
			    request->p2 = static_cast<void*>(&mc->yPos);
                        // Make sure the "square selector" mesh is visible and position it over the current tile.

                    }
                }
            }
        }

        // If we are drawing with the brush in the map editor.
        if (mc->mLMouseDown && mc->mDragType == tileBrushSelection && !isInGame())
        {
            // Loop over the square region surrounding current mouse location and either set the tile type of the affected tiles or create new ones.
            Tile *currentTile;
            std::vector<Tile*> affectedTiles;
            int radiusSquared = mCurrentTileRadius * mCurrentTileRadius;

            for (int i = -1 * (mCurrentTileRadius - 1); i <= (mCurrentTileRadius
                    - 1); ++i)
            {
                for (int j = -1 * (mCurrentTileRadius - 1); j
                        <= (mCurrentTileRadius - 1); ++j)
                {
                    // Check to see if the current location falls inside a circle with a radius of mCurrentTileRadius.
                    int distSquared = i * i + j * j;

                    if (distSquared > radiusSquared)
                        continue;

                    currentTile = mc->gameMap->getTile(mc->xPos + i, mc->yPos + j);

                    // Check to see if the current tile already exists.
                    if (currentTile != NULL)
                    {
                        // It does exist so set its type and fullness.
                        affectedTiles.push_back(currentTile);
                        currentTile->setType((Tile::TileType)mCurrentTileType);
                        currentTile->setFullness((Tile::TileType)mCurrentFullness);
                    }
                    else
                    {
                        // The current tile does not exist so we need to create it.
                        //currentTile = new Tile;
			stringstream ss;

			ss.str(std::string());
			ss << "Level";
			ss << "_";
			ss << mc->xPos + 1;
			ss << "_";
			ss << mc->yPos + 1;


                        currentTile = new Tile(mc->xPos + i, mc->yPos + j,
                                               (Tile::TileType)mCurrentTileType, (Tile::TileType)mCurrentFullness);
                        currentTile->setName(ss.str());
                        mc->gameMap->addTile(currentTile);
                        currentTile->createMesh();
                    }
                }
            }

            // Add any tiles which border the affected region to the affected tiles list
            // as they may alo want to switch meshes to optimize polycount now too.
            std::vector<Tile*> borderingTiles = mc->gameMap->tilesBorderedByRegion(
                                                    affectedTiles);

            affectedTiles.insert(affectedTiles.end(), borderingTiles.begin(),
                                 borderingTiles.end());

            // Loop over all the affected tiles and force them to examine their
            // neighbors.  This allows them to switch to a mesh with fewer
            // polygons if some are hidden by the neighbors.
            for (unsigned int i = 0; i < affectedTiles.size(); ++i)
                affectedTiles[i]->setFullness(affectedTiles[i]->getFullness());
        }

        // If we are dragging a map light we need to update its position to the current x-y location.
        if (mc->mLMouseDown && mc->mDragType == mapLight && !isInGame())
        {
            MapLight* tempMapLight = mc->gameMap->getMapLight(draggedMapLight);

            if (tempMapLight != NULL)
                tempMapLight->setPosition(mc->xPos, mc->yPos, tempMapLight->getPosition().z);
        }

        if (arg.state.Z.rel > 0)
        {
            if (mc->mKeyboard->isModifierDown(OIS::Keyboard::Ctrl))
            {
                CameraManager::getSingleton().move(CameraManager::moveDown);
            }
            else
            {
                mc->gameMap->getLocalPlayer()->rotateCreaturesInHand(1);
            }
        }
        else
            if (arg.state.Z.rel < 0)
            {
                if (mc->mKeyboard->isModifierDown(OIS::Keyboard::Ctrl))
                {
                    CameraManager::getSingleton().move(CameraManager::moveUp);
                }
                else
                {
                    mc->gameMap->getLocalPlayer()->rotateCreaturesInHand(-1);
                }
            }
            else
            {
                CameraManager::getSingleton().stopZooming();
            }
    }

    //CameraManager::getSingleton().move(CameraManager::fullStop);

    if (!(mc->directionKeyPressed || mc->mDragType == rotateAxisX || mc->mDragType == rotateAxisY))
    {

        if (arg.state.X.abs == 0)
            CameraManager::getSingleton().move(CameraManager::moveLeft);
        else
            CameraManager::getSingleton().move(CameraManager::stopLeft);

        if (arg.state.X.abs ==  arg.state.width)
            CameraManager::getSingleton().move(CameraManager::moveRight);
        else
            CameraManager::getSingleton().move(CameraManager::stopRight);

        if (arg.state.Y.abs == 0)
            CameraManager::getSingleton().move(CameraManager::moveForward);
        else
            CameraManager::getSingleton().move(CameraManager::stopForward);

        if (arg.state.Y.abs ==  arg.state.height)
            CameraManager::getSingleton().move(CameraManager::moveBackward);
        else
            CameraManager::getSingleton().move(CameraManager::stopBackward);
    }


    //  cerr << arg.state.X.abs <<" " ;
    //  cerr << arg.state.Y.abs <<" " ;
    //  cerr << arg.state.Z.abs <<"\n" ;
    //  cerr << arg.state.width <<"\n" ;
    //  cerr << arg.state.height <<"\n" ;

    //CameraManager::getSingleton().move(CameraManager::moveBackward);


    return true;








}
bool EditorMode::mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id){

    CEGUI::System::getSingleton().injectMouseButtonDown(
        Gui::getSingletonPtr()->convertButton(id));

    // If the mouse press is on a CEGUI window ignore it
    CEGUI::Window *tempWindow =
        CEGUI::System::getSingleton().getWindowContainingMouse();

    if (tempWindow != 0 && tempWindow->getName().compare("TOOLSPALETE") != 0)
    {
        mc->mouseDownOnCEGUIWindow = true;
        return true;
    }
    else
    {
        mc->mouseDownOnCEGUIWindow = false;
    }

    Ogre::RaySceneQueryResult &result = ODFrameListener::getSingleton().doRaySceneQuery(arg);

    Ogre::RaySceneQueryResult::iterator itr = result.begin();

    std::string resultName;

    // Left mouse button down

    if (id == OIS::MB_Left)
    {
        mc->mLMouseDown = true;
        mc->mLStartDragX = mc->xPos;
        mc->mLStartDragY = mc->yPos;

	if(arg.state.Y.abs < 0.1*arg.state.height || arg.state.Y.abs > 0.9*arg.state.height){
		mc->mDragType=rotateAxisX;
		return true;
	    }

	else if(arg.state.X.abs > 0.9*arg.state.width || arg.state.X.abs < 0.1*arg.state.width){
		mc->mDragType=rotateAxisY;
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

                    if(!isInGame()) // if in the Map Editor:  Begin dragging the creature
                    {
                        Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
                        mSceneMgr->getEntity("SquareSelector")->setVisible(
                            false);

                        draggedCreature = resultName.substr(
                                              ((std::string) "Creature_").size(),
                                              resultName.size());
                        Ogre::SceneNode *node = mSceneMgr->getSceneNode(
                                                    draggedCreature + "_node");
                        ODFrameListener::getSingleton().getCreatureSceneNode()->removeChild(node);
                        mSceneMgr->getSceneNode("Hand_node")->addChild(node);
                        node->setPosition(0, 0, 0);
                        mc->mDragType = creature;

                        SoundEffectsHelper::getSingleton().playInterfaceSound(
                            SoundEffectsHelper::PICKUP);

                        return true;
                    }
                }

            }

            ++itr;
        }

        // If no creatures are under the  mouse run through the list again to check for lights
        if (!isInGame())
        {
            //FIXME: These other code blocks that loop over the result list should probably use this same loop structure.
            itr = result.begin();

            while (itr != result.end())
            {
                if (itr->movable != NULL)
                {
                    resultName = itr->movable->getName();

                    if (resultName.find("MapLightIndicator_") != std::string::npos)
                    {
                        mc->mDragType = mapLight;
                        draggedMapLight = resultName.substr(
                                              ((std::string) "MapLightIndicator_").size(),
                                              resultName.size());

                        SoundEffectsHelper::getSingleton().playInterfaceSound(
                            SoundEffectsHelper::PICKUP);

                        return true;
                    }
                }

                ++itr;
            }
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
                    mc->mDragType = tileSelection;

                    // If we are in the map editor, use a brush selection if it has been activated.

                    if (!isInGame() && mBrushMode)
                    {
                        mc->mDragType = tileBrushSelection;
                    }

                    // If we have selected a room type to add to the map, use a addNewRoom drag type.
                    if (mc->gameMap->getLocalPlayer()->getNewRoomType() != Room::nullRoomType)
                    {
                        mc->mDragType = addNewRoom;
                    }

                    // If we have selected a trap type to add to the map, use a addNewTrap drag type.
                    else
                        if (mc->gameMap->getLocalPlayer()->getNewTrapType() != Trap::nullTrapType)
                        {
                            mc->mDragType = addNewTrap;
                        }

                    break;
                }
            }

            ++itr;
        }


    }

    // Right mouse button down
    if (id == OIS::MB_Right)
    {
        mc->mRMouseDown = true;
        mc->mRStartDragX = mc->xPos;
        mc->mRStartDragY = mc->yPos;

        // Stop creating rooms, traps, etc.
        mc->mDragType = nullDragType;
        mc->gameMap->getLocalPlayer()->setNewRoomType(Room::nullRoomType);
        mc->gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
        TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "");

        // If we right clicked with the mouse over a valid map tile, try to drop a creature onto the map.
        Tile *curTile = mc->gameMap->getTile(mc->xPos, mc->yPos);

        if (curTile != NULL)
        {
            mc->gameMap->getLocalPlayer()->dropCreature(curTile);

            if (mc->gameMap->getLocalPlayer()->numCreaturesInHand() > 0)
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
                    Creature* tempCreature = mc->gameMap->getCreature(resultName.substr(
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
bool EditorMode::mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id){

    CEGUI::System::getSingleton().injectMouseButtonUp(
        Gui::getSingletonPtr()->convertButton(id));


    // If the mouse press was on a CEGUI window ignore it

    if (mc->mouseDownOnCEGUIWindow)
        return true;

    for (int jj = 0; jj < mc->gameMap->mapSizeY; ++jj)
	{
	    for (int ii = 0; ii < mc->gameMap->mapSizeX; ++ii)
		{
		    mc->gameMap->getTile(ii,jj)->setSelected(false,mc->gameMap->getLocalPlayer());
		}
	}


    // Unselect all tiles
    // for (TileMap_t::iterator itr = mc->gameMap->firstTile(), last = mc->gameMap->lastTile();
    //         itr != last; ++itr)
    // {
    //     itr->second->setSelected(false,mc->gameMap->getLocalPlayer());
    // }

    // Left mouse button up
    if (id == OIS::MB_Left)
    {
	if(mc->mDragType == rotateAxisX){

	    mc->mDragType=nullDragType;
	}
	else if(mc->mDragType == rotateAxisY){

	    mc->mDragType=nullDragType;
	}

        // Check to see if we are moving a creature
        else if (mc->mDragType == creature)
        {
            if (!isInGame())
            {
                Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
                Ogre::SceneNode *node = mSceneMgr->getSceneNode(draggedCreature + "_node");
                mSceneMgr->getSceneNode("Hand_node")->removeChild(node);
                ODFrameListener::getSingleton().getCreatureSceneNode()->addChild(node);
                mc->mDragType = nullDragType;
                mc->gameMap->getCreature(draggedCreature)->setPosition(Ogre::Vector3(mc->xPos, mc->yPos, 0));
            }
        }

        // Check to see if we are dragging a map light.
        else
            if (mc->mDragType == mapLight)
            {
                if (!isInGame())
                {
                    MapLight *tempMapLight = mc->gameMap->getMapLight(draggedMapLight);

                    if (tempMapLight != NULL)
                    {
                        tempMapLight->setPosition(mc->xPos, mc->yPos, tempMapLight->getPosition().z);
                    }
                }
            }

        // Check to see if we are dragging out a selection of tiles or creating a new room
            else
                if (mc->mDragType == tileSelection || mc->mDragType == addNewRoom ||
                        mc->mDragType == addNewTrap)
                {
                    //TODO: move to own function.

                    // Loop over the valid tiles in the affected region.  If we are doing a tileSelection (changing the tile type and fullness) this
                    // loop does that directly.  If, instead, we are doing an addNewRoom, this loop prunes out any tiles from the affectedTiles vector
                    // which cannot have rooms placed on them, then if the player has enough gold, etc to cover the selected tiles with the given room
                    // the next loop will actually create the room.  A similar pruning is done for traps.
                    std::vector<Tile*> affectedTiles = mc->gameMap->rectangularRegion(mc->xPos,
                                                       mc->yPos, mc->mLStartDragX, mc->mLStartDragY);
                    std::vector<Tile*>::iterator itr = affectedTiles.begin();

                    while (itr != affectedTiles.end())
                    {
                        Tile *currentTile = *itr;

                        // If we are dragging out tiles.

                        if (mc->mDragType == tileSelection)
                        {
                            // See if we are in a game or not

                            if(!isInGame())
                            {
                                // In the map editor:  Fill the current tile with the new value
                                currentTile->setType((Tile::TileType)mCurrentTileType);
                                currentTile->setFullness((Tile::TileType)mCurrentFullness);
                            }
                        }
                        else // if(mc->mDragType == ExampleFrameListener::addNewRoom || mc->mDragType == ExampleFrameListener::addNewTrap)
                        {
                            // If the tile already contains a room, prune it from the list of affected tiles.
                            if (!currentTile->isBuildableUpon())
                            {
                                itr = affectedTiles.erase(itr);
                                continue;
                            }

 
                            if (!isInGame())                            // We are in the map editor
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
                    if (mc->mDragType == addNewRoom && !affectedTiles.empty())
                    {
                        Room* newRoom = Room::buildRoom(mc->gameMap, mc->gameMap->getLocalPlayer()->getNewRoomType(),
                                                        affectedTiles, mc->gameMap->getLocalPlayer(), !isInGame());

                        if (newRoom == NULL)
                        {
                            //Not enough money
                            //TODO:  play sound or something.
                        }
                    }

                    // If we are adding new traps the above loop will have pruned out the tiles not eligible
                    // for adding traps to.  This block then actually adds traps to the remaining tiles.
                    else
                        if (mc->mDragType == addNewTrap && !affectedTiles.empty())
                        {
                            Trap* newTrap = Trap::buildTrap(mc->gameMap, mc->gameMap->getLocalPlayer()->getNewTrapType(),
                                                            affectedTiles, mc->gameMap->getLocalPlayer(), !isInGame());

                            if (newTrap == NULL)
                            {
                                //Not enough money
                                //TODO:  play sound or something.
                            }
                        }

                    // Add the tiles which border the affected region to the affectedTiles vector since they may need to have their meshes changed.
                    std::vector<Tile*> borderTiles = mc->gameMap->tilesBorderedByRegion(
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

        mc->mLMouseDown = false;
    }

    // Right mouse button up
    if (id == OIS::MB_Right)
    {
        mc->mRMouseDown = false;
    }

    return true;




}
bool EditorMode::keyPressed     (const OIS::KeyEvent &arg){
   //TODO: do this (and the others isInGame() in here) by GameState
    if (mc->frameListener->isTerminalActive())
    {

    }
    else
    {
        //inject key to Gui
        CEGUI::System* sys = CEGUI::System::getSingletonPtr();
        sys->injectKeyDown(arg.key);
        sys->injectChar(arg.text);

        CameraManager& camMgr = CameraManager::getSingleton();

        switch (arg.key)
        {

            case OIS::KC_GRAVE:

            case OIS::KC_F12:
		progressMode(ModeManager::CONSOLE);
                mc->frameListener->setTerminalActive(true);
                Console::getSingleton().setVisible(true);
                mc->mKeyboard->setTextTranslation(OIS::Keyboard::Ascii);
                break;

            case OIS::KC_LEFT:

            case OIS::KC_A:
                mc->directionKeyPressed = true;

                camMgr.move(camMgr.moveLeft); // Move left
                break;

            case OIS::KC_RIGHT:

            case OIS::KC_D:
                mc->directionKeyPressed = true;
                camMgr.move(camMgr.moveRight); // Move right
                break;

            case OIS::KC_UP:

            case OIS::KC_W:

                mc->directionKeyPressed = true;
                camMgr.move(camMgr.moveForward); // Move forward
                break;

            case OIS::KC_DOWN:

            case OIS::KC_S:

                mc->directionKeyPressed = true;
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

                //Toggle mCurrentTileType

            case OIS::KC_R:

                if (!isInGame())
                {
                    mCurrentTileType = Tile::nextTileType((Tile::TileType)mCurrentTileType);
                    std::stringstream tempSS("");
                    tempSS << "Tile type:  " << Tile::tileTypeToString(
                        (Tile::TileType)mCurrentTileType);
                    ODApplication::MOTD = tempSS.str();
                }

                break;

                //Decrease brush radius

            case OIS::KC_COMMA:

                if (!isInGame())
                {
                    if (mCurrentTileRadius > 1)
                    {
                        --mCurrentTileRadius;
                    }

                    ODApplication::MOTD = "Brush size:  " + Ogre::StringConverter::toString(

                                              mCurrentTileRadius);
                }

                break;

                //Increase brush radius

            case OIS::KC_PERIOD:

                if (!isInGame())
                {
                    if (mCurrentTileRadius < 10)
                    {
                        ++mCurrentTileRadius;
                    }

                    ODApplication::MOTD = "Brush size:  " + Ogre::StringConverter::toString(

                                              mCurrentTileRadius);
                }

                break;

                //Toggle mBrushMode

            case OIS::KC_B:

                if (!isInGame())
                {
                    mBrushMode = !mBrushMode;
                    ODApplication::MOTD = (mBrushMode)
                                          ? "Brush mode turned on"
                                          : "Brush mode turned off";
                }

                break;

                //Toggle mCurrentFullness

            case OIS::KC_T:
                // If we are not in a game.

                if (!isInGame())
                {
                    mCurrentFullness = Tile::nextTileFullness(mCurrentFullness);
                    ODApplication::MOTD = "Tile fullness:  " + Ogre::StringConverter::toString(
                                              mCurrentFullness);
                }


                break;

                // Quit the game

            case OIS::KC_ESCAPE:
                //MapLoader::writeGameMapToFile(std::string("levels/Test.level") + ".out", *mc->gameMap);
                //mc->frameListener->requestExit();
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
bool EditorMode::keyReleased    (const OIS::KeyEvent &arg){
    CEGUI::System::getSingleton().injectKeyUp(arg.key);

    if (!mc->frameListener->isTerminalActive())
    {
        CameraManager& camMgr = CameraManager::getSingleton();

        switch (arg.key)
        {

            case OIS::KC_LEFT:

            case OIS::KC_A:
                mc->directionKeyPressed = false;
                camMgr.move(camMgr.stopLeft);
                break;

            case OIS::KC_RIGHT:

            case OIS::KC_D:
                mc->directionKeyPressed = false;
                camMgr.move(camMgr.stopRight);
                break;

            case OIS::KC_UP:

            case OIS::KC_W:
                mc->directionKeyPressed = false;
                camMgr.move(camMgr.stopForward);
                break;

            case OIS::KC_DOWN:

            case OIS::KC_S:
                mc->directionKeyPressed = false;
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
void EditorMode::handleHotkeys  (OIS::KeyCode keycode){

    //keycode minus two because the codes are shifted by two against the actual number
    unsigned int keynumber = keycode - 2;

    if (mc->mKeyboard->isModifierDown(OIS::Keyboard::Shift))
    {
        mc->hotkeyLocationIsValid[keynumber] = true;
        mc->hotkeyLocation[keynumber] = CameraManager::getSingleton().getCameraViewTarget();
    }
    else
    {
        if (mc->hotkeyLocationIsValid[keynumber])
        {
            CameraManager::getSingleton().flyTo(mc->hotkeyLocation[keynumber]);
        }
    }



}
bool EditorMode::isInGame(){
    //TODO: this exact function is also in ODFrameListener, replace it too after GameState works
    //TODO - we should use a bool or something, not the sockets for this.
    return (Socket::serverSocket != NULL || Socket::clientSocket != NULL);
    //return GameState::getSingletonPtr()->getApplicationState() == GameState::ApplicationState::GAME;


}
    
void EditorMode::giveFocus(){

    mc->mMouse->setEventCallback(this);
    mc->mKeyboard->setEventCallback(this);
}
