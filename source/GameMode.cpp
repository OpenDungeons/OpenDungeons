/*!
 * \date:  02 July 2011
 * \author StefanP.MUC
 *
 *  Copyright (C) 2011-2014  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

GameMode::GameMode(ModeManager *modeManager):
    AbstractApplicationMode(modeManager, ModeManager::GAME),
    mDigSetBool(false),
    mGameMap(ODFrameListener::getSingletonPtr()->getGameMap())
{
    // TODO: Permit loading any level.
    // Read in the default game map
    mGameMap->LoadLevel("levels/Test.level");
}

GameMode::~GameMode()
{
}

/*! \brief Process the mouse movement event.
 *
 * The function does a raySceneQuery to determine what object the mouse is over
 * to handle things like dragging out selections of tiles and selecting
 * creatures.
 */
bool GameMode::mouseMoved(const OIS::MouseEvent &arg)
{

    CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition((float)arg.state.X.abs, (float)arg.state.Y.abs);

    if (!isConnected())
        return true;

    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    InputManager* inputManager = mModeManager->getInputManager();
    CameraManager* cm = frameListener->cm;

    if (frameListener->isTerminalActive())
        return true;

    //If we have a room or trap (or later spell) selected, show what we
    //have selected
    //TODO: This should be changed, or combined with an icon or something later.
    if (mGameMap->getLocalPlayer()->getNewRoomType() || mGameMap->getLocalPlayer()->getNewTrapType())
    {
        TextRenderer::getSingleton().moveText(ODApplication::POINTER_INFO_STRING,
                                                (Ogre::Real)(arg.state.X.abs + 30), (Ogre::Real)arg.state.Y.abs);
    }

    Ogre::RaySceneQueryResult& result = ODFrameListener::getSingleton().doRaySceneQuery(arg);

    Ogre::RaySceneQueryResult::iterator itr = result.begin();
    Ogre::RaySceneQueryResult::iterator end = result.end();
    Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
    std::string resultName;

    if(inputManager->mDragType == rotateAxisX)
    {
        cm->move(CameraManager::randomRotateX, arg.state.X.rel);
    }

    else if(inputManager->mDragType == rotateAxisY)
    {
        cm->move(CameraManager::randomRotateY, arg.state.Y.rel);
    }
    else if (inputManager->mDragType == tileSelection || inputManager->mDragType == addNewRoom
             || inputManager->mDragType == nullDragType)
    {
        // Since this is a tile selection query we loop over the result set and look for the first object which is actually a tile.
        for (; itr != end; ++itr)
        {
            if (itr->movable == NULL)
                continue;

            // Check to see if the current query result is a tile.
            resultName = itr->movable->getName();

            if (resultName.find("Level_") == std::string::npos)
                continue;

            //Make the mouse light follow the mouse
            //TODO - we should make a pointer to the light or something.
            Ogre::RaySceneQuery* rq = frameListener->getRaySceneQuery();
            Ogre::Real dist = itr->distance;
            Ogre::Vector3 point = rq->getRay().getPoint(dist);
            mSceneMgr->getLight("MouseLight")->setPosition(point.x, point.y, 4.0);

            // Get the x-y coordinates of the tile.
            sscanf(resultName.c_str(), "Level_%i_%i", &inputManager->mXPos, &inputManager->mYPos);
            RenderRequest *request = new RenderRequest;

            request->type = RenderRequest::showSquareSelector;
            request->p = static_cast<void*>(&inputManager->mXPos);
            request->p2 = static_cast<void*>(&inputManager->mYPos);

            // Add the request to the queue of rendering operations to be performed before the next frame.
            RenderManager::queueRenderRequest(request);

            if (inputManager->mLMouseDown)
            {
                // Loop over the tiles in the rectangular selection region and set their setSelected flag accordingly.
                //TODO: This function is horribly inefficient, it should loop over a rectangle selecting tiles by x-y coords
                // rather than the reverse that it is doing now.
                std::vector<Tile*> affectedTiles = mGameMap->rectangularRegion(inputManager->mXPos,
                                                                               inputManager->mYPos,
                                                                               inputManager->mLStartDragX,
                                                                               inputManager->mLStartDragY);


                for (int jj = 0; jj < mGameMap->getMapSizeY(); ++jj)
                {
                    for (int ii = 0; ii < mGameMap->getMapSizeX(); ++ii)
                    {
                        mGameMap->getTile(ii,jj)->setSelected(false, mGameMap->getLocalPlayer());
                    }
                }

                for( std::vector<Tile*>::iterator itr =  affectedTiles.begin(); itr != affectedTiles.end(); ++itr )
                {
                    (*itr)->setSelected(true, mGameMap->getLocalPlayer());
                }
            }

            break;
        }
    }
    else
    {
        // We are dragging a creature but we want to loop over the result set to find the first tile entry,
        // we do this to get the current x-y location of where the "square selector" should be drawn.
        for (; itr != end; ++itr)
        {
            if (itr->movable == NULL)
                continue;

            // Check to see if the current query result is a tile.
            resultName = itr->movable->getName();

            if (resultName.find("Level_") == std::string::npos)
                continue;

            // Get the x-y coordinates of the tile.
            sscanf(resultName.c_str(), "Level_%i_%i", &inputManager->mXPos, &inputManager->mYPos);

            RenderRequest *request = new RenderRequest;

            request->type = RenderRequest::showSquareSelector;
            request->p = static_cast<void*>(&inputManager->mXPos);
            request->p2 = static_cast<void*>(&inputManager->mYPos);
            // Make sure the "square selector" mesh is visible and position it over the current tile.
        }
    }



    if (arg.state.Z.rel > 0)
    {
        if (getKeyboard()->isModifierDown(OIS::Keyboard::Ctrl))
        {
            mGameMap->getLocalPlayer()->rotateCreaturesInHand(1);
        }
        else
        {
            cm->move(CameraManager::moveDown);
        }
    }
    else if (arg.state.Z.rel < 0)
    {
        if (getKeyboard()->isModifierDown(OIS::Keyboard::Ctrl))
        {
            mGameMap->getLocalPlayer()->rotateCreaturesInHand(-1);
        }
        else
        {
            cm->move(CameraManager::moveUp);
        }
    }
    else
    {
        cm->stopZooming();
    }

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

    if (!isConnected())
        return true;

    CEGUI::Window *tempWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getWindowContainingMouse();

    InputManager* inputManager = mModeManager->getInputManager();

    // If the mouse press is on a CEGUI window ignore it
    if (tempWindow != NULL && tempWindow->getName().compare("Root") != 0)
    {
        inputManager->mMouseDownOnCEGUIWindow = true;
        return true;
    }

    inputManager->mMouseDownOnCEGUIWindow = false;

    Ogre::RaySceneQueryResult &result = ODFrameListener::getSingleton().doRaySceneQuery(arg);
    Ogre::RaySceneQueryResult::iterator itr = result.begin();
    std::string resultName;

    if (id == OIS::MB_Middle)
    {
        // See if the mouse is over any creatures
        for (;itr != result.end(); ++itr)
        {
            if (itr->movable == NULL)
                continue;

            resultName = itr->movable->getName();

            if (resultName.find("Creature_") == std::string::npos)
                continue;

            Creature* tempCreature = mGameMap->getCreature(resultName.substr(
                                            ((std::string) "Creature_").size(), resultName.size()));

            if (tempCreature != NULL)
                tempCreature->createStatsWindow();

            return true;

        }
        return true;
    }

    // Right mouse button down
    if (id == OIS::MB_Right)
    {
        inputManager->mRMouseDown = true;
        inputManager->mRStartDragX = inputManager->mXPos;
        inputManager->mRStartDragY = inputManager->mYPos;

        // Stop creating rooms, traps, etc.
        inputManager->mDragType = nullDragType;
        mGameMap->getLocalPlayer()->setNewRoomType(Room::nullRoomType);
        mGameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
        TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "");

        // If we right clicked with the mouse over a valid map tile, try to drop a creature onto the map.
        Tile *curTile = mGameMap->getTile(inputManager->mXPos, inputManager->mYPos);

        if (curTile == NULL)
            return true;

        mGameMap->getLocalPlayer()->dropCreature(curTile);

        if (mGameMap->getLocalPlayer()->numCreaturesInHand() > 0)
        {
            SoundEffectsHelper::getSingleton().playInterfaceSound(SoundEffectsHelper::DROP);
        }
    }

    if (id != OIS::MB_Left)
        return true;

    // Left mouse button down
    inputManager->mLMouseDown = true;
    inputManager->mLStartDragX = inputManager->mXPos;
    inputManager->mLStartDragY = inputManager->mYPos;

    if(arg.state.Y.abs < 0.1 * arg.state.height || arg.state.Y.abs > 0.9 * arg.state.height)
    {
        inputManager->mDragType = rotateAxisX;
        return true;
    }
    else if(arg.state.X.abs > 0.9 * arg.state.width || arg.state.X.abs < 0.1 * arg.state.width)
    {
        inputManager->mDragType = rotateAxisY;
        return true;
    }

    CameraManager* cm = ODFrameListener::getSingletonPtr()->cm;

    // See if the mouse is over any creatures
    for (;itr != result.end(); ++itr)
    {
        if (itr->movable == NULL)
            continue;

        resultName = itr->movable->getName();

        if (resultName.find("Creature_") == std::string::npos)
            continue;

        // Pick the creature up and put it in our hand
        if(inputManager->mExpectCreatureClick)
        {
            progressMode(ModeManager::FPP);
            const string& tmp_name =  (itr->movable->getName());
            std::cerr << tmp_name.substr(9, tmp_name.size()) << std::endl;
            cm->setFPPCamera(mGameMap->getCreature(tmp_name.substr( 9, tmp_name.size())));
            cm->setActiveCameraNode("FPP");
            cm->setActiveCamera("FPP");

            inputManager->mExpectCreatureClick =false;
        }
        else
        {
            // through away everything before the '_' and then copy the rest into 'array'
            char array[255];
            std::stringstream tempSS;
            tempSS.str(resultName);
            tempSS.getline(array, sizeof(array), '_');
            tempSS.getline(array, sizeof(array));

            Creature *currentCreature = mGameMap->getCreature(array);

            if (currentCreature != 0 && currentCreature->getColor() == mGameMap->getLocalPlayer()->getSeat()->getColor())
            {
                mGameMap->getLocalPlayer()->pickUpCreature(currentCreature);
                SoundEffectsHelper::getSingleton().playInterfaceSound(SoundEffectsHelper::PICKUP);
                return true;
            }
            else
            {
                LogManager::getSingleton().logMessage("Tried to pick up another players creature, or creature was 0");
            }
        }
    }

    // If no creatures or lights are under the  mouse run through the list again to check for tiles
    for (itr = result.begin(); itr != result.end(); ++itr)
    {
        if (itr->movable == NULL)
            continue;

        if (resultName.find("Level_") == std::string::npos)
            continue;

        // Start by assuming this is a tileSelection drag.
        inputManager->mDragType = tileSelection;

        // If we have selected a room type to add to the map, use a addNewRoom drag type.
        if (mGameMap->getLocalPlayer()->getNewRoomType() != Room::nullRoomType)
        {
            inputManager->mDragType = addNewRoom;
        }

        // If we have selected a trap type to add to the map, use a addNewTrap drag type.
        else if (mGameMap->getLocalPlayer()->getNewTrapType() != Trap::nullTrapType)
        {
            inputManager->mDragType = addNewTrap;
        }

        break;
    }

    // If we are in a game we store the opposite of whether this tile is marked for diggin or not, this allows us to mark tiles
    // by dragging out a selection starting from an unmarcked tile, or unmark them by starting the drag from a marked one.
    Tile *tempTile = mGameMap->getTile(inputManager->mXPos, inputManager->mYPos);

    if (tempTile != NULL)
        mDigSetBool = !(tempTile->getMarkedForDigging(mGameMap->getLocalPlayer()));

    return true;
}

/*! \brief Handle mouse button releases.
 *
 * Finalize the selection of tiles or drop a creature when the user releases the mouse button.
 */
bool GameMode::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
                Gui::getSingletonPtr()->convertButton(id));

    InputManager* inputManager = mModeManager->getInputManager();

    // If the mouse press was on a CEGUI window ignore it
    if (inputManager->mMouseDownOnCEGUIWindow)
        return true;

    if (!isConnected())
        return true;

    // Unselect all tiles
    for (int jj = 0; jj < mGameMap->getMapSizeY(); ++jj)
    {
        for (int ii = 0; ii < mGameMap->getMapSizeX(); ++ii)
        {
            mGameMap->getTile(ii,jj)->setSelected(false, mGameMap->getLocalPlayer());
        }
    }

    // Right mouse button up
    if (id == OIS::MB_Right)
    {
        inputManager->mRMouseDown = false;
        return true;
    }

    if (id != OIS::MB_Left)
        return true;

    // Left mouse button up
    inputManager->mLMouseDown = false;

    switch(inputManager->mDragType)
    {
        default:
            return true;

        case rotateAxisX:
        case rotateAxisY:
            inputManager->mDragType = nullDragType;
            return true;

        case creature:
            return true;

        case mapLight:
            return true;

        // When either selecting a tile, adding room or a trap
        // we do what's next.
        case tileSelection:
        case addNewRoom:
        case addNewTrap:
            break;
    }


    // Loop over the valid tiles in the affected region.  If we are doing a tileSelection (changing the tile type and fullness) this
    // loop does that directly.  If, instead, we are doing an addNewRoom, this loop prunes out any tiles from the affectedTiles vector
    // which cannot have rooms placed on them, then if the player has enough gold, etc to cover the selected tiles with the given room
    // the next loop will actually create the room.  A similar pruning is done for traps.
    std::vector<Tile*> affectedTiles = mGameMap->rectangularRegion(inputManager->mXPos,
                                                                   inputManager->mYPos,
                                                                   inputManager->mLStartDragX,
                                                                   inputManager->mLStartDragY);
    std::vector<Tile*>::iterator itr = affectedTiles.begin();

    while (itr != affectedTiles.end())
    {
        Tile *currentTile = *itr;

        // If we are dragging out tiles.

        if (inputManager->mDragType == tileSelection)
        {
            //See if the tile can be marked for digging.
            if (currentTile->isDiggable())
            {
                if (Socket::serverSocket != NULL)
                {
                    // On the server: Just mark the tile for digging.
                    currentTile->setMarkedForDigging(mDigSetBool, mGameMap->getLocalPlayer());
                }
                else
                {
                    // On the client:  Inform the server about our choice
                    ClientNotification *clientNotification = new ClientNotification;
                    clientNotification->mType = ClientNotification::markTile;
                    clientNotification->mP = currentTile;
                    clientNotification->mFlag = mDigSetBool;

                    sem_wait(&ClientNotification::mClientNotificationQueueLockSemaphore);
                    ClientNotification::mClientNotificationQueue.push_back(clientNotification);
                    sem_post(&ClientNotification::mClientNotificationQueueLockSemaphore);

                    sem_post(&ClientNotification::mClientNotificationQueueSemaphore);

                    currentTile->setMarkedForDigging(mDigSetBool, mGameMap->getLocalPlayer());

                }

                SoundEffectsHelper::getSingleton().playInterfaceSound(SoundEffectsHelper::DIGSELECT, false);
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

            // If the currentTile is not empty and claimed for my color, then remove it from the affectedTiles vector.
            if (!(currentTile->getFullness() < 1
                    && currentTile->getType() == Tile::claimed
                    && currentTile->colorDouble > 0.99
                    && currentTile->getColor() == mGameMap->getLocalPlayer()->getSeat()->color))
            {
                itr = affectedTiles.erase(itr);
                continue;
            }
        }

        ++itr;
    }

    // If we are adding new rooms the above loop will have pruned out the tiles not eligible
    // for adding rooms to.  This block then actually adds rooms to the remaining tiles.
    if (inputManager->mDragType == addNewRoom && !affectedTiles.empty())
    {
        Room* newRoom = Room::buildRoom(mGameMap, mGameMap->getLocalPlayer()->getNewRoomType(),
                                        affectedTiles, mGameMap->getLocalPlayer(), false);

        if (newRoom == NULL)
        {
            //Not enough money
            //TODO:  play sound or something.
        }
    }

    // If we are adding new traps the above loop will have pruned out the tiles not eligible
    // for adding traps to.  This block then actually adds traps to the remaining tiles.
    else if (inputManager->mDragType == addNewTrap && !affectedTiles.empty())
    {
        Trap* newTrap = Trap::buildTrap(mGameMap, mGameMap->getLocalPlayer()->getNewTrapType(),
                                        affectedTiles, mGameMap->getLocalPlayer(), false);

        if (newTrap == NULL)
        {
            //Not enough money
            //TODO:  play sound or something.
        }
    }

    // Add the tiles which border the affected region to the affectedTiles vector since they may need to have their meshes changed.
    std::vector<Tile*> borderTiles = mGameMap->tilesBorderedByRegion(affectedTiles);

    affectedTiles.insert(affectedTiles.end(), borderTiles.begin(), borderTiles.end());

    // Loop over all the affected tiles and force them to examine their neighbors.  This allows
    // them to switch to a mesh with fewer polygons if some are hidden by the neighbors, etc.
    itr = affectedTiles.begin();

    while (itr != affectedTiles.end())
    {
        (*itr)->setFullness((*itr)->getFullness());
        ++itr;
    }

    return true;
}

/*! \brief Handle the keyboard input.
 *
 */
bool GameMode::keyPressed(const OIS::KeyEvent &arg)
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    if (frameListener->isTerminalActive())
        return true;

    // Inject key to Gui
    CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyDown((CEGUI::Key::Scan) arg.key);
    CEGUI::System::getSingleton().getDefaultGUIContext().injectChar(arg.text);

    CameraManager& camMgr = *(frameListener->cm);
    InputManager* inputManager = mModeManager->getInputManager();

    switch (arg.key)
    {
    case OIS::KC_F11:
        frameListener->toggleDebugInfo();
        break;

    case OIS::KC_GRAVE:
    case OIS::KC_F12:
        progressMode(ModeManager::CONSOLE);
        frameListener->setTerminalActive(true);
        Console::getSingleton().setVisible(true);
        getKeyboard()->setTextTranslation(OIS::Keyboard::Ascii);
        break;

    case OIS::KC_LEFT:
    case OIS::KC_A:
        inputManager->mDirectionKeyPressed = true;
        camMgr.move(camMgr.moveLeft); // Move left
        break;

    case OIS::KC_RIGHT:
    case OIS::KC_D:
        inputManager->mDirectionKeyPressed = true;
        camMgr.move(camMgr.moveRight); // Move right
        break;

    case OIS::KC_UP:
    case OIS::KC_W:
        inputManager->mDirectionKeyPressed = true;
        camMgr.move(camMgr.moveForward); // Move forward
        break;

    case OIS::KC_DOWN:
    case OIS::KC_S:
        inputManager->mDirectionKeyPressed = true;
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

    case OIS::KC_T:
        if(isConnected()) // If we are in a game.
        {
            Seat* tempSeat = mGameMap->getLocalPlayer()->getSeat();
            camMgr.flyTo(Ogre::Vector3((Ogre::Real)tempSeat->startingX,
                                                   (Ogre::Real)tempSeat->startingY,
                                                   (Ogre::Real)0.0));
        }
        break;

    // Quit the game
    case OIS::KC_ESCAPE:
        //MapLoader::writeGameMapToFile(std::string("levels/Test.level") + ".out", *mMc->gameMap);
        //mMc->frameListener->requestExit();
        regressMode();
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

    return true;
}

/*! \brief Process the key up event.
 *
 * When a key is released during normal gamplay the camera movement may need to be stopped.
 */
bool GameMode::keyReleased(const OIS::KeyEvent &arg)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyUp((CEGUI::Key::Scan) arg.key);

    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    if (frameListener->isTerminalActive())
        return true;

    CameraManager& camMgr = *(frameListener->cm);
    InputManager* inputManager = mModeManager->getInputManager();

    switch (arg.key)
    {
    case OIS::KC_LEFT:
    case OIS::KC_A:
        inputManager->mDirectionKeyPressed = false;
        camMgr.move(camMgr.stopLeft);
        break;

    case OIS::KC_RIGHT:
    case OIS::KC_D:
        inputManager->mDirectionKeyPressed = false;
        camMgr.move(camMgr.stopRight);
        break;

    case OIS::KC_UP:
    case OIS::KC_W:
        inputManager->mDirectionKeyPressed = false;
        camMgr.move(camMgr.stopForward);
        break;

    case OIS::KC_DOWN:
    case OIS::KC_S:
        inputManager->mDirectionKeyPressed = false;
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
    InputManager* inputManager = mModeManager->getInputManager();
    CameraManager* cm = ODFrameListener::getSingletonPtr()->cm;

    //keycode minus two because the codes are shifted by two against the actual number
    unsigned int keynumber = keycode - 2;

    if (getKeyboard()->isModifierDown(OIS::Keyboard::Shift))
    {
        inputManager->mHotkeyLocationIsValid[keynumber] = true;
        inputManager->mHotkeyLocation[keynumber] = cm->getCameraViewTarget();
    }
    else
    {
        if (inputManager->mHotkeyLocationIsValid[keynumber])
        {
            cm->flyTo(inputManager->mHotkeyLocation[keynumber]);
        }
    }
}

void GameMode::onFrameStarted(const Ogre::FrameEvent& evt)
{
    CameraManager* cm = ODFrameListener::getSingletonPtr()->cm;
    cm->moveCamera(evt.timeSinceLastFrame);

    mGameMap->getMiniMap()->draw();
    mGameMap->getMiniMap()->swap();
}

void GameMode::onFrameEnded(const Ogre::FrameEvent& evt)
{

}
