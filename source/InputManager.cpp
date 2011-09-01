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

#include <algorithm>
#include <vector>
#include <string>

#include "Globals.h"
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

#include "InputManager.h"

template<> InputManager* Ogre::Singleton<InputManager>::ms_Singleton = 0;

InputManager::~InputManager()
{
    LogManager::getSingleton().logMessage("Destroying input objects.");
    mInputManager->destroyInputObject(mMouse);
    mInputManager->destroyInputObject(mKeyboard);
    OIS::InputManager::destroyInputSystem(mInputManager);
    mInputManager = 0;
}

InputManager::InputManager(GameMap* gameMap) :
            frameListener(ODFrameListener::getSingletonPtr()),
            mLMouseDown(false),
            mRMouseDown(false),
            mBrushMode(false),
            digSetBool(false),
            mStatsOn(false),
            mCurrentFullness(100),
            mCurrentTileRadius(1),
            xPos(0),
            yPos(0),
            mLStartDragX(0),
            mLStartDragY(0),
            mRStartDragX(0),
            mRStartDragY(0),
            mCurrentTileType(Tile::dirt),
            mDragType(nullDragType),
            gameMap(gameMap)
{
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
    mMouse = static_cast<OIS::Mouse*>(mInputManager->createInputObject(
            OIS::OISMouse, true));
    mMouse->setEventCallback(this);
}


/*! \brief Process the mouse movement event.
 *
 * The function does a raySceneQuery to determine what object the mouse is over
 * to handle things like dragging out selections of tiles and selecting
 * creatures.
 */
bool InputManager::mouseMoved(const OIS::MouseEvent &arg)
{
    CEGUI::System::getSingleton().injectMousePosition(arg.state.X.abs, arg.state.Y.abs);
    //TODO: do this (and the others isInGame() in here) by GameState
    if(frameListener->isTerminalActive())
    {
        Console::getSingleton().onMouseMoved(arg);
    }
    else
    {
        //If we have a room or trap (or later spell) selected, show what we
        //have selected
        //TODO: This should be changed, or combined with an icon or something later.
        if (gameMap->getLocalPlayer()->getNewRoomType() || gameMap->getLocalPlayer()->getNewTrapType())
        {
            TextRenderer::getSingleton().moveText(ODApplication::POINTER_INFO_STRING,
                    arg.state.X.abs + 30, arg.state.Y.abs);
        }

        Ogre::RaySceneQueryResult& result = ODFrameListener::getSingleton().doRaySceneQuery(arg);
        Ogre::RaySceneQueryResult::iterator itr = result.begin();
        Ogre::RaySceneQueryResult::iterator end = result.end();
        Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
        std::string resultName = "";


        if (mDragType == tileSelection || mDragType == addNewRoom || mDragType == nullDragType)
        {
            // Since this is a tile selection query we loop over the result set and look for the first object which is actually a tile.
            for(; itr != end; ++itr)
            {
                if (itr->movable != NULL)
                {
                    // Check to see if the current query result is a tile.
                    resultName = itr->movable->getName();
                    if (resultName.find("Level_") != std::string::npos)
                    {
                        //Make the mouse light follow the mouse
                        //TODO - we should make a pointer to the light or something.
                        Ogre::RaySceneQuery* rq = frameListener->getRaySceneQuery();
                        Ogre::Real dist = itr->distance;
                        Ogre::Vector3 point = rq->getRay().getPoint(dist);
                        mSceneMgr->getLight("MouseLight")->setPosition(point.x, point.y, 4.0);

                        // Get the x-y coordinates of the tile.
                        sscanf(resultName.c_str(), "Level_%i_%i", &xPos, &yPos);

                        // Make sure the "square selector" mesh is visible and position it over the current tile.
                        mSceneMgr->getEntity("SquareSelector")->setVisible(true);
                        mSceneMgr->getSceneNode("SquareSelectorNode")->setPosition(
                                xPos, yPos, 0);
                        //mSceneMgr->getLight("MouseLight")->setPosition(xPos, yPos, 2.0);

                        if (mLMouseDown)
                        {
                            // Loop over the tiles in the rectangular selection region and set their setSelected flag accordingly.
                            //TODO: This function is horribly inefficient, it should loop over a rectangle selecting tiles by x-y coords rather than the reverse that it is doing now.
                            std::vector<Tile*> affectedTiles = gameMap->rectangularRegion(xPos,
                                                yPos, mLStartDragX, mLStartDragY);
                            for(TileMap_t::iterator itr = gameMap->firstTile(), last = gameMap->lastTile();
                                                            itr != last; ++itr)
                            {
                                if(std::find(affectedTiles.begin(), affectedTiles.end(), itr->second) != affectedTiles.end())
                                {
                                    itr->second->setSelected(true);
                                }
                                else
                                {
                                    itr->second->setSelected(false);
                                }
                            }
                        }

                        if (mRMouseDown)
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
            for(; itr != end; ++itr)
            {
                if (itr->movable != NULL)
                {
                    // Check to see if the current query result is a tile.
                    resultName = itr->movable->getName();
                    if (resultName.find("Level_") != std::string::npos)
                    {
                        // Get the x-y coordinates of the tile.
                        sscanf(resultName.c_str(), "Level_%i_%i", &xPos, &yPos);

                        // Make sure the "square selector" mesh is visible and position it over the current tile.
                        mSceneMgr->getEntity("SquareSelector")->setVisible(true);
                        mSceneMgr->getSceneNode("SquareSelectorNode")->setPosition(
                                xPos, yPos, 0);
                    }
                }
            }
        }

        // If we are drawing with the brush in the map editor.
        if (mLMouseDown && mDragType == tileBrushSelection && !isInGame())
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

                    currentTile = gameMap->getTile(xPos + i, yPos + j);

                    // Check to see if the current tile already exists.
                    if (currentTile != NULL)
                    {
                        // It does exist so set its type and fullness.
                        affectedTiles.push_back(currentTile);
                        currentTile->setType(mCurrentTileType);
                        currentTile->setFullness(mCurrentFullness);
                    }
                    else
                    {
                        // The current tile does not exist so we need to create it.
                        //currentTile = new Tile;
                        char tempArray[255];
                        snprintf(tempArray, sizeof(tempArray), "Level_%3i_%3i",
                                xPos + i, yPos + j);
                        currentTile = new Tile(xPos + i, yPos + j,
                                mCurrentTileType, mCurrentFullness);
                        currentTile->name = tempArray;
                        gameMap->addTile(currentTile);
                        currentTile->createMesh();
                    }
                }
            }

            // Add any tiles which border the affected region to the affected tiles list
            // as they may alo want to switch meshes to optimize polycount now too.
            std::vector<Tile*> borderingTiles = gameMap->tilesBorderedByRegion(
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
        if (mLMouseDown && mDragType == mapLight && !isInGame())
        {
            MapLight* tempMapLight = gameMap->getMapLight(draggedMapLight);
            if (tempMapLight != NULL)
                tempMapLight->setPosition(xPos, yPos, tempMapLight->getPosition().z);
        }

        if(arg.state.Z.rel > 0)
        {
            if(mKeyboard->isModifierDown(OIS::Keyboard::Ctrl))
            {
                CameraManager::getSingleton().move(CameraManager::moveDown);
            }
            else
            {
                gameMap->getLocalPlayer()->rotateCreaturesInHand(1);
            }
        }
        else if(arg.state.Z.rel < 0)
        {
            if(mKeyboard->isModifierDown(OIS::Keyboard::Ctrl))
            {
                CameraManager::getSingleton().move(CameraManager::moveUp);
            }
            else
            {
                gameMap->getLocalPlayer()->rotateCreaturesInHand(-1);
            }
        }
        else
        {
            CameraManager::getSingleton().stopZooming();
        }
    }

    return true;
}

/*! \brief Handle mouse clicks.
 *
 * This function does a ray scene query to determine what is under the mouse
 * and determines whether a creature or a selection of tiles, is being dragged.
 */
bool InputManager::mousePressed(const OIS::MouseEvent &arg,
        OIS::MouseButtonID id)
{
    CEGUI::System::getSingleton().injectMouseButtonDown(
            Gui::getSingletonPtr()->convertButton(id));

    // If the mouse press is on a CEGUI window ignore it
    CEGUI::Window *tempWindow =
            CEGUI::System::getSingleton().getWindowContainingMouse();

    if (tempWindow != 0 && tempWindow->getName().compare("Root") != 0)
    {
        mouseDownOnCEGUIWindow = true;
        return true;
    }
    else
    {
        mouseDownOnCEGUIWindow = false;
    }

    Ogre::RaySceneQueryResult &result = ODFrameListener::getSingleton().doRaySceneQuery(arg);
    Ogre::RaySceneQueryResult::iterator itr = result.begin();

    std::string resultName;

    // Left mouse button down
    if (id == OIS::MB_Left)
    {
        mLMouseDown = true;
        mLStartDragX = xPos;
        mLStartDragY = yPos;

        // See if the mouse is over any creatures
        while (itr != result.end())
        {
            if (itr->movable != NULL)
            {
                resultName = itr->movable->getName();

                if (resultName.find("Creature_") != std::string::npos)
                {
                    // if in a game:  Pick the creature up and put it in our hand
                    if (isInGame())
                    {
                        // through away everything before the '_' and then copy the rest into 'array'
                        char array[255];
                        std::stringstream tempSS;
                        tempSS.str(resultName);
                        tempSS.getline(array, sizeof(array), '_');
                        tempSS.getline(array, sizeof(array));

                        Creature *currentCreature = gameMap->getCreature(array);
                        if (currentCreature != 0 && currentCreature->color
                                == gameMap->getLocalPlayer()->getSeat()->color)
                        {
                            gameMap->getLocalPlayer()->pickUpCreature(currentCreature);
                            SoundEffectsHelper::getSingleton().playInterfaceSound(
                                    SoundEffectsHelper::PICKUP);
                            return true;
                        }
                    }
                    else // if in the Map Editor:  Begin dragging the creature
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
                        mDragType = creature;

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
                        mDragType = mapLight;
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
                    mDragType = tileSelection;

                    // If we are in the map editor, use a brush selection if it has been activated.
                    if (!isInGame() && mBrushMode)
                    {
                        mDragType = tileBrushSelection;
                    }

                    // If we have selected a room type to add to the map, use a addNewRoom drag type.
                    if (gameMap->getLocalPlayer()->getNewRoomType() != Room::nullRoomType)
                    {
                        mDragType = addNewRoom;
                    }

                    // If we have selected a trap type to add to the map, use a addNewTrap drag type.
                    else if (gameMap->getLocalPlayer()->getNewTrapType() != Trap::nullTrapType)
                    {
                        mDragType = addNewTrap;
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
            Tile *tempTile = gameMap->getTile(xPos, yPos);
            if (tempTile != NULL)
            {
                digSetBool = !(tempTile->getMarkedForDigging(gameMap->getLocalPlayer()));
            }
        }
    }

    // Right mouse button down
    if (id == OIS::MB_Right)
    {
        mRMouseDown = true;
        mRStartDragX = xPos;
        mRStartDragY = yPos;

        // Stop creating rooms, traps, etc.
        mDragType = nullDragType;
        gameMap->getLocalPlayer()->setNewRoomType(Room::nullRoomType);
        gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
        TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "");

        // If we right clicked with the mouse over a valid map tile, try to drop a creature onto the map.
        Tile *curTile = gameMap->getTile(xPos, yPos);
        if (curTile != NULL)
        {
            gameMap->getLocalPlayer()->dropCreature(curTile);
            if (gameMap->getLocalPlayer()->numCreaturesInHand() > 0)
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
                    Creature* tempCreature = gameMap->getCreature(resultName.substr(
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
bool InputManager::mouseReleased(const OIS::MouseEvent &arg,
        OIS::MouseButtonID id)
{
    CEGUI::System::getSingleton().injectMouseButtonUp(
            Gui::getSingletonPtr()->convertButton(id));

    // If the mouse press was on a CEGUI window ignore it
    if (mouseDownOnCEGUIWindow)
        return true;

    // Unselect all tiles
    for(TileMap_t::iterator itr = gameMap->firstTile(), last = gameMap->lastTile();
            itr != last; ++itr)
    {
        itr->second->setSelected(false);
    }

    // Left mouse button up
    if (id == OIS::MB_Left)
    {
        // Check to see if we are moving a creature
        if (mDragType == creature)
        {
            if (!isInGame())
            {
                Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
                Ogre::SceneNode *node = mSceneMgr->getSceneNode(draggedCreature + "_node");
                mSceneMgr->getSceneNode("Hand_node")->removeChild(node);
                ODFrameListener::getSingleton().getCreatureSceneNode()->addChild(node);
                mDragType = nullDragType;
                gameMap->getCreature(draggedCreature)->setPosition(xPos, yPos, 0);
            }
        }

        // Check to see if we are dragging a map light.
        else if (mDragType == mapLight)
        {
            if (!isInGame())
            {
                MapLight *tempMapLight = gameMap->getMapLight(draggedMapLight);
                if (tempMapLight != NULL)
                {
                    tempMapLight->setPosition(xPos, yPos, tempMapLight->getPosition().z);
                }
            }
        }

        // Check to see if we are dragging out a selection of tiles or creating a new room
        else if (mDragType == tileSelection || mDragType == addNewRoom ||
                mDragType == addNewTrap)
        {
            //TODO: move to own function.
            
            // Loop over the valid tiles in the affected region.  If we are doing a tileSelection (changing the tile type and fullness) this
            // loop does that directly.  If, instead, we are doing an addNewRoom, this loop prunes out any tiles from the affectedTiles vector
            // which cannot have rooms placed on them, then if the player has enough gold, etc to cover the selected tiles with the given room
            // the next loop will actually create the room.  A similar pruning is done for traps.
            std::vector<Tile*> affectedTiles = gameMap->rectangularRegion(xPos,
                    yPos, mLStartDragX, mLStartDragY);
            std::vector<Tile*>::iterator itr = affectedTiles.begin();
            while (itr != affectedTiles.end())
            {
                Tile *currentTile = *itr;

                // If we are dragging out tiles.
                if (mDragType == tileSelection)
                {
                    // See if we are in a game or not
                    if (isInGame())
                    {
                        //See if the tile can be marked for digging.
                        if (currentTile->isDiggable())
                        {
                            if (serverSocket != NULL)
                            {
                                // On the server:  Just mark the tile for digging.
                                currentTile->setMarkedForDigging(digSetBool,
                                        gameMap->getLocalPlayer());
                            }
                            else
                            {
                                // On the client:  Inform the server about our choice
                                ClientNotification *clientNotification =
                                        new ClientNotification;
                                clientNotification->type
                                        = ClientNotification::markTile;
                                clientNotification->p = currentTile;
                                clientNotification->flag = digSetBool;

                                sem_wait(&clientNotificationQueueLockSemaphore);
                                clientNotificationQueue.push_back(
                                        clientNotification);
                                sem_post(&clientNotificationQueueLockSemaphore);

                                sem_post(&clientNotificationQueueSemaphore);

                                currentTile->setMarkedForDigging(digSetBool,
                                        gameMap->getLocalPlayer());

                            }

                            SoundEffectsHelper::getSingleton().playInterfaceSound(
                                    SoundEffectsHelper::DIGSELECT, false);
                        }
                    }
                    else
                    {
                        // In the map editor:  Fill the current tile with the new value
                        currentTile->setType(mCurrentTileType);
                        currentTile->setFullness(mCurrentFullness);
                    }
                }
                else // if(mDragType == ExampleFrameListener::addNewRoom || mDragType == ExampleFrameListener::addNewTrap)
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
                                        == gameMap->getLocalPlayer()->getSeat()->color))
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
            if (mDragType == addNewRoom && !affectedTiles.empty())
            {
                Room* newRoom = Room::buildRoom(gameMap, gameMap->getLocalPlayer()->getNewRoomType(),
                    affectedTiles, gameMap->getLocalPlayer(), !isInGame());

                if(newRoom == NULL)
                {
                    //Not enough money
                    //TODO:  play sound or something.
                }
            }
            // If we are adding new traps the above loop will have pruned out the tiles not eligible
            // for adding traps to.  This block then actually adds traps to the remaining tiles.
            else if (mDragType == addNewTrap && !affectedTiles.empty())
            {
                Trap* newTrap = Trap::buildTrap(gameMap, gameMap->getLocalPlayer()->getNewTrapType(),
                    affectedTiles, gameMap->getLocalPlayer(), !isInGame());
                if(newTrap == NULL)
                {
                    //Not enough money
                    //TODO:  play sound or something.
                }
            }

            // Add the tiles which border the affected region to the affectedTiles vector since they may need to have their meshes changed.
            std::vector<Tile*> borderTiles = gameMap->tilesBorderedByRegion(
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

        mLMouseDown = false;
    }

    // Right mouse button up
    if (id == OIS::MB_Right)
    {
        mRMouseDown = false;
    }

    return true;
}

/*! \brief Handle the keyboard input.
 *
 */
bool InputManager::keyPressed(const OIS::KeyEvent &arg)
{
    //TODO: do this (and the others isInGame() in here) by GameState
    if(frameListener->isTerminalActive())
    {
        Console::getSingleton().onKeyPressed(arg);
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
                frameListener->setTerminalActive(true);
                Console::getSingleton().setVisible(true);
                mKeyboard->setTextTranslation(OIS::Keyboard::Ascii);
                break;

            case OIS::KC_LEFT:
            case OIS::KC_A:
                camMgr.move(camMgr.moveLeft); // Move left
                break;

            case OIS::KC_RIGHT:
            case OIS::KC_D:
                camMgr.move(camMgr.moveRight); // Move right
                break;

            case OIS::KC_UP:
            case OIS::KC_W:
                camMgr.move(camMgr.moveForward); // Move forward
                break;

            case OIS::KC_DOWN:
            case OIS::KC_S:
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
                    mCurrentTileType = Tile::nextTileType(mCurrentTileType);
                    std::stringstream tempSS("");
                    tempSS << "Tile type:  " << Tile::tileTypeToString(
                            mCurrentTileType);
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
                else // If we are in a game.
                {
                    Seat* tempSeat = gameMap->getLocalPlayer()->getSeat();
                    CameraManager::getSingleton().flyTo(Ogre::Vector3(
                            tempSeat->startingX, tempSeat->startingY, 0.0));
                }
                break;

                // Quit the game
            case OIS::KC_ESCAPE:
                MapLoader::writeGameMapToFile(std::string("levels/Test.level")+ ".out", *gameMap);
                frameListener->requestExit();
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
bool InputManager::keyReleased(const OIS::KeyEvent &arg)
{
    CEGUI::System::getSingleton().injectKeyUp(arg.key);

    if(!frameListener->isTerminalActive())
    {
        CameraManager& camMgr = CameraManager::getSingleton();
        switch (arg.key)
        {
            case OIS::KC_LEFT:
            case OIS::KC_A:
                camMgr.move(camMgr.stopLeft);
                break;

            case OIS::KC_RIGHT:
            case OIS::KC_D:
                camMgr.move(camMgr.stopRight);
                break;

            case OIS::KC_UP:
            case OIS::KC_W:
                camMgr.move(camMgr.stopForward);
                break;

            case OIS::KC_DOWN:
            case OIS::KC_S:
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
void InputManager::handleHotkeys(OIS::KeyCode keycode)
{
    //keycode minus two because the codes are shifted by two against the actual number
    unsigned int keynumber = keycode - 2;

    if (mKeyboard->isModifierDown(OIS::Keyboard::Shift))
    {
        hotkeyLocationIsValid[keynumber] = true;
        hotkeyLocation[keynumber] = CameraManager::getSingleton().getCameraViewTarget();
    }
    else
    {
        if (hotkeyLocationIsValid[keynumber])
        {
            CameraManager::getSingleton().flyTo(hotkeyLocation[keynumber]);
        }
    }
}

/*! \brief Check if we are in editor mode
 *
 */
bool InputManager::isInGame()
{
    //TODO: this exact function is also in ODFrameListener, replace it too after GameState works
    //TODO - we should use a bool or something, not the sockets for this.
    return (serverSocket != NULL || clientSocket != NULL);
    //return GameState::getSingletonPtr()->getApplicationState() == GameState::ApplicationState::GAME;
}
