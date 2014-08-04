/*
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

#include "EditorMode.h"

#include "MapLoader.h"
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
#include "MusicPlayer.h"

#include <OgreEntity.h>

#include <algorithm>
#include <vector>
#include <string>
#include <cstdio>

EditorMode::EditorMode(ModeManager* modeManager):
    AbstractApplicationMode(modeManager, ModeManager::EDITOR),
    mCurrentTileType(Tile::dirt),
    mCurrentFullness(100.0),
    mGameMap(ODFrameListener::getSingletonPtr()->getGameMap()),
    mMouseX(0),
    mMouseY(0),
    mMouseLight(NULL)
{
    // TODO: Permit loading any level.
    // Read in the default game map
    mGameMap->LoadLevel("levels/Test.level");

    // Set per default the input on the map
    mModeManager->getInputManager()->mMouseDownOnCEGUIWindow = false;

    // Keep track of the mouse light object
    Ogre::SceneManager* sceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
    mMouseLight = sceneMgr->getLight("MouseLight");

    // Start on the map center
    ODFrameListener::getSingleton().cm->setCameraPosition(Ogre::Vector3((Ogre::Real)mGameMap->getMapSizeX() / 2,
                                                                        (Ogre::Real)mGameMap->getMapSizeY() / 2, MAX_CAMERA_Z));
}

EditorMode::~EditorMode()
{
}

void EditorMode::activate()
{
    // Loads the corresponding Gui sheet.
    Gui::getSingleton().loadGuiSheet(Gui::editorMenu);

    giveFocus();

    // Stop the game music.
    MusicPlayer::getSingleton().stop();
}

bool EditorMode::mouseMoved(const OIS::MouseEvent &arg)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition((float)arg.state.X.abs, (float)arg.state.Y.abs);

    if (ODFrameListener::getSingletonPtr()->isTerminalActive())
        return true;


    TextRenderer::getSingleton().moveText(ODApplication::POINTER_INFO_STRING,
                                          (Ogre::Real)(arg.state.X.abs + 30), (Ogre::Real)arg.state.Y.abs);

    handleMouseWheel(arg);

    // Handles drag type logic
    handleMouseMovedDragType(arg);

    return true;
}

void EditorMode::handleMouseWheel(const OIS::MouseEvent& arg)
{
    CameraManager* cm = ODFrameListener::getSingleton().cm;

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
}

void EditorMode::handleCursorPositionUpdate()
{
    InputManager* inputManager = mModeManager->getInputManager();

    // Don't update if we didn't actually change the tile coordinate.
    if (mMouseX == inputManager->mXPos && mMouseY == inputManager->mYPos)
        return;

    // Updates mouse position for other functions.
    mMouseX = inputManager->mXPos;
    mMouseY = inputManager->mYPos;

    // Make the mouse light follow the mouse
    mMouseLight->setPosition((Ogre::Real)mMouseX, (Ogre::Real)mMouseY, 2.0);

    // Make the square selector follow the mouse
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::showSquareSelector;
    request->p = static_cast<void*>(&mMouseX);
    request->p2 = static_cast<void*>(&mMouseY);
    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request); // NOTE: will delete the request member for us.

    updateCursorText();
}

void EditorMode::handleMouseMovedDragType(const OIS::MouseEvent &arg)
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    CameraManager* cm = frameListener->cm;
    if (!cm)
        return;

    Ogre::RaySceneQueryResult& result = frameListener->doRaySceneQuery(arg);

    Ogre::RaySceneQueryResult::iterator itr = result.begin();
    Ogre::RaySceneQueryResult::iterator end = result.end();

    InputManager* inputManager = mModeManager->getInputManager();

    switch(inputManager->mDragType)
    {
    default:
    case tileSelection:
    case addNewRoom:
    case nullDragType:
        // Since this is a tile selection query we loop over the result set and look for the first object which is actually a tile.
        for (; itr != end; ++itr)
        {
            if (itr->movable == NULL)
                continue;

            // Check to see if the current query result is a tile.
            std::string resultName = itr->movable->getName();

            if (resultName.find("Level_") == std::string::npos)
                continue;

            // Updates the x-y coordinates of the tile.
            sscanf(resultName.c_str(), "Level_%i_%i", &inputManager->mXPos, &inputManager->mYPos);
            handleCursorPositionUpdate();

            // If we don't drag anything, there is no affected tiles to compute.
            if (!inputManager->mLMouseDown || inputManager->mDragType == nullDragType)
                return;

            // Handle when dragging using the left mouse button

            // Loop over the tiles in the rectangular selection region and set their type
            std::vector<Tile*> affectedTiles = mGameMap->rectangularRegion(inputManager->mXPos,
                                                                           inputManager->mYPos,
                                                                           inputManager->mLStartDragX,
                                                                           inputManager->mLStartDragY);

            for(std::vector<Tile*>::iterator itr = affectedTiles.begin(); itr != affectedTiles.end(); ++itr)
            {
                (*itr)->setSelected(true, mGameMap->getLocalPlayer());
            }
            break;
        }
        return;

    case mapLight:
    {
        // If we are dragging a map light we need to update its position to the current x-y location.
        if (!inputManager->mLMouseDown)
            return;

        MapLight* tempMapLight = mGameMap->getMapLight(mDraggedMapLight);

        if (tempMapLight != NULL)
            tempMapLight->setPosition((Ogre::Real)inputManager->mXPos, (Ogre::Real)inputManager->mYPos,
                                      tempMapLight->getPosition().z);

        return;
    }

    } // switch drag type
}

bool EditorMode::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    CEGUI::GUIContext& ctxt = CEGUI::System::getSingleton().getDefaultGUIContext();
    ctxt.injectMouseButtonDown(Gui::getSingletonPtr()->convertButton(id));

    // If the mouse press is on a CEGUI window ignore it
    CEGUI::Window *tempWindow = ctxt.getWindowContainingMouse();

    InputManager* inputManager = mModeManager->getInputManager();

    if (tempWindow != 0 && tempWindow->getName().compare("EDITORGUI") != 0)
    {
        inputManager->mMouseDownOnCEGUIWindow = true;
        return true;
    }

    inputManager->mMouseDownOnCEGUIWindow = false;

    Ogre::RaySceneQueryResult &result = ODFrameListener::getSingleton().doRaySceneQuery(arg);
    Ogre::RaySceneQueryResult::iterator itr = result.begin();

    std::string resultName;

    // Left mouse button down
    if (id == OIS::MB_Left)
    {
        inputManager->mLMouseDown = true;
        inputManager->mLStartDragX = inputManager->mXPos;
        inputManager->mLStartDragY = inputManager->mYPos;

        // See if the mouse is over any creatures
        for (; itr != result.end(); ++itr)
        {
            if (itr->movable == NULL)
                continue;

            resultName = itr->movable->getName();

            if (resultName.find(Creature::CREATURE_PREFIX) == std::string::npos)
                continue;

            // Begin dragging the creature
            RenderManager* rdrMgr = RenderManager::getSingletonPtr();
            Ogre::SceneManager* sceneMgr = rdrMgr->getSceneManager();
            sceneMgr->getEntity("SquareSelector")->setVisible(false);

            mDraggedCreature = resultName.substr(
                                    Creature::CREATURE_PREFIX.length(),
                                    resultName.length());
            Ogre::SceneNode *node = sceneMgr->getSceneNode(mDraggedCreature + "_node");
            rdrMgr->getCreatureSceneNode()->removeChild(node);
            sceneMgr->getSceneNode("Hand_node")->addChild(node);
            node->setPosition(0, 0, 0);
            inputManager->mDragType = creature;

            updateCursorText();

            SoundEffectsHelper::getSingleton().playInterfaceSound(SoundEffectsHelper::PICKUP);

            return true;
        }

        // If no creatures are under the  mouse run through the list again to check for lights
        //TODO: These other code blocks that loop over the result list should probably use this same loop structure.
        for (itr = result.begin(); itr != result.end(); ++itr)
        {
            if (itr->movable == NULL)
                continue;

            resultName = itr->movable->getName();

            if (resultName.find("MapLightIndicator_") == std::string::npos)
                continue;

            inputManager->mDragType = mapLight;
            mDraggedMapLight = resultName.substr(((std::string) "MapLightIndicator_").size(),
                                                    resultName.size());
            updateCursorText();

            SoundEffectsHelper::getSingleton().playInterfaceSound(SoundEffectsHelper::PICKUP);

            return true;
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
            else
            {
                // If we have selected a trap type to add to the map, use a addNewTrap drag type.
                if (mGameMap->getLocalPlayer()->getNewTrapType() != Trap::nullTrapType)
                    inputManager->mDragType = addNewTrap;
            }

            updateCursorText();

            break;
        }
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

        updateCursorText();
    }

    if (id == OIS::MB_Middle)
    {
        // See if the mouse is over any creatures
        for (itr = result.begin(); itr != result.end(); ++itr)
        {
            if (itr->movable == NULL)
                continue;

            resultName = itr->movable->getName();

            if (resultName.find(Creature::CREATURE_PREFIX) == std::string::npos)
                continue;

            Creature* tempCreature = mGameMap->getCreature(resultName.substr(
                Creature::CREATURE_PREFIX.length(), resultName.length()));

            if (tempCreature != NULL)
                tempCreature->createStatsWindow();

            return true;
        }
    }
    return true;
}

bool EditorMode::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(Gui::getSingletonPtr()->convertButton(id));

    InputManager* inputManager = mModeManager->getInputManager();

    // If the mouse press was on a CEGUI window ignore it
    if (inputManager->mMouseDownOnCEGUIWindow)
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

    // Left mouse button up
    if (id != OIS::MB_Left)
        return true;

    inputManager->mLMouseDown = false;

    // Check to see if we are moving a creature
    if (inputManager->mDragType == creature)
    {
        // If the user tries to drop the creature on a wall tile, then we put it back to its former place.
        // Same goes when we drop it on a non-existing tile.
        Tile* droppedTile = mGameMap->getTile(inputManager->mXPos, inputManager->mYPos);
        Creature* droppedCreature = mGameMap->getCreature(mDraggedCreature);
        Tile::TileClearType passability = droppedCreature->getDefinition()->getTilePassability();

        Ogre::Real newXPos = (Ogre::Real)inputManager->mXPos;
        Ogre::Real newYPos = (Ogre::Real)inputManager->mYPos;
        if (droppedTile == NULL || droppedTile->getTilePassability() != passability)
        {
            newXPos = (Ogre::Real)inputManager->mLStartDragX;
            newYPos = (Ogre::Real)inputManager->mLStartDragY;
        }

        // Remove the creature node from the selector and back to the game
        RenderManager* rdrMgr = RenderManager::getSingletonPtr();
        Ogre::SceneManager* sceneMgr = rdrMgr->getSceneManager();
        Ogre::SceneNode* node = sceneMgr->getSceneNode(mDraggedCreature + "_node");
        sceneMgr->getSceneNode("Hand_node")->removeChild(node);
        rdrMgr->getCreatureSceneNode()->addChild(node);

        inputManager->mDragType = nullDragType;
        droppedCreature->setPosition(Ogre::Vector3(newXPos, newYPos, (Ogre::Real)0.0));
        updateCursorText();
        SoundEffectsHelper::getSingleton().playInterfaceSound(SoundEffectsHelper::DROP);
        return true;
    }

    // Check to see if we are dragging a map light.
    if (inputManager->mDragType == mapLight)
    {
        MapLight *tempMapLight = mGameMap->getMapLight(mDraggedMapLight);

        if (tempMapLight != NULL)
            tempMapLight->setPosition((Ogre::Real)inputManager->mXPos, (Ogre::Real)inputManager->mYPos,
                                      tempMapLight->getPosition().z);
        inputManager->mDragType = nullDragType;
        updateCursorText();
        return true;
    }

    // Check to see if we are dragging out a selection of tiles or creating a new room
    if (inputManager->mDragType != tileSelection && inputManager->mDragType != addNewRoom
        && inputManager->mDragType != addNewTrap)
        return true;

    // If we are dragging out tiles.
    if (inputManager->mDragType == tileSelection)
    {
        // Loop over the square region surrounding current mouse location
        // and either set the tile type of the affected tiles or create new ones.
        std::vector<Tile*> affectedTiles;

        // Get selection range
        int startX = inputManager->mLStartDragX;
        int startY = inputManager->mLStartDragY;
        int endX = inputManager->mXPos;
        int endY = inputManager->mYPos;

        // Handle possible inverted selections
        if (startX > endX)
        {
            int temp = startX;
            startX = endX;
            endX = temp;
        }
        if (startY > endY)
        {
            int temp = startY;
            startY = endY;
            endY = temp;
        }

        for (int x = startX; x <= endX; ++x)
        {
            for (int y = startY; y <= endY; ++y)
            {
                Tile* currentTile = mGameMap->getTile(x, y);

                // Check to see if the current tile already exists.
                if (currentTile != NULL)
                {
                    // Do not add a wall upon a creature
                    if (currentTile->numCreaturesInCell() > 0)
                        continue;

                    // It does exist so set its type and fullness.
                    affectedTiles.push_back(currentTile);
                    currentTile->setType(mCurrentTileType);
                    currentTile->setFullness(mCurrentFullness);
                    continue;
                }

                // The current tile does not exist so we need to create it.
                std::stringstream ss;

                ss.str(std::string());
                ss << "Level_";
                ss << x;
                ss << "_";
                ss << y;

                Tile* myTile = new Tile(mGameMap, x, y, mCurrentTileType, mCurrentFullness);
                myTile->setName(ss.str());
                myTile->createMesh();
                mGameMap->addTile(myTile);
                affectedTiles.push_back(myTile);
            }
        }

        mGameMap->refreshBorderingTilesOf(affectedTiles);
        inputManager->mDragType = nullDragType;
        updateCursorText();
        return true;
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

        if(inputManager->mDragType != addNewRoom && inputManager->mDragType != addNewTrap)
            continue;

        // If the tile already contains a room, prune it from the list of affected tiles.
        if (!currentTile->isBuildableUpon())
        {
            itr = affectedTiles.erase(itr);
            continue;
        }

        // If the currentTile is not empty, then remove it from the affectedTiles vector.
        if (currentTile->getFullness() >= 1)
        {
            itr = affectedTiles.erase(itr);
            continue;
        }

        ++itr;
    }

    // If we are adding new rooms the above loop will have pruned out the tiles not eligible
    // for adding rooms to.  This block then actually adds rooms to the remaining tiles.
    if (inputManager->mDragType == addNewRoom && !affectedTiles.empty())
    {
        Room* newRoom = Room::createRoom(mGameMap, mGameMap->getLocalPlayer()->getNewRoomType(),
            affectedTiles, mGameMap->getLocalPlayer()->getSeat()->getColor());
        Room::setupRoom(mGameMap, newRoom, mGameMap->getLocalPlayer());
    }

    // If we are adding new traps the above loop will have pruned out the tiles not eligible
    // for adding traps to.  This block then actually adds traps to the remaining tiles.
    else if (inputManager->mDragType == addNewTrap && !affectedTiles.empty())
    {
        Trap* newTrap = Trap::createTrap(mGameMap, mGameMap->getLocalPlayer()->getNewTrapType(),
            affectedTiles, mGameMap->getLocalPlayer()->getSeat());

        Trap::setupTrap(mGameMap, newTrap, mGameMap->getLocalPlayer());
    }

    mGameMap->refreshBorderingTilesOf(affectedTiles);
    inputManager->mDragType = nullDragType;
    updateCursorText();
    return true;
}

void EditorMode::updateCursorText()
{
    // Gets the current action from the drag type
    std::stringstream textSS("");
    switch(mModeManager->getInputManager()->mDragType)
    {
    default:
    case nullDragType:
    case tileSelection:
        textSS << "Tile type: " << Tile::tileTypeToString(mCurrentTileType);
        break;
    case creature:
        textSS << "Creature: " << mDraggedCreature;
        break;
    case mapLight:
        textSS << "Light: " << mDraggedMapLight;
        break;
    case addNewRoom:
    case addNewTrap:
        //TODO
        break;
    }

    // Tells the current tile type and fullness;
    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, textSS.str());

    // Update the fullness info
    CEGUI::Window *posWin = Gui::getSingletonPtr()->getGuiSheet(Gui::editorMenu)->getChild(Gui::EDITOR_FULLNESS);
    textSS.str("");
    textSS << "Tile Fullness (T): " << mCurrentFullness << "%";
    posWin->setText(textSS.str());

    // Update the cursor position
    posWin = Gui::getSingletonPtr()->getGuiSheet(Gui::editorMenu)->getChild(Gui::EDITOR_CURSOR_POS);
    textSS.str("");
    textSS << "Cursor: x: " << mMouseX << ", y: " << mMouseY;
    posWin->setText(textSS.str());
}

bool EditorMode::keyPressed(const OIS::KeyEvent &arg)
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
        mModeManager->requestConsoleMode();
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

    case OIS::KC_Q:
        camMgr.move(camMgr.rotateLeft); // Turn left
        break;

    case OIS::KC_E:
        camMgr.move(camMgr.rotateRight); // Turn right
        break;

    case OIS::KC_PGUP:
        camMgr.move(camMgr.rotateUp); // Tilt up
        break;

    case OIS::KC_PGDOWN:
        camMgr.move(camMgr.rotateDown); // Tilt down
        break;

    case OIS::KC_HOME:
        camMgr.move(camMgr.moveUp); // Move up
        break;

    case OIS::KC_END:
        camMgr.move(camMgr.moveDown); // Move down
        break;

    //Toggle mCurrentTileType
    case OIS::KC_R:
    {
        mCurrentTileType = Tile::nextTileType(mCurrentTileType);
        updateCursorText();
    }
        break;

    //Toggle mCurrentFullness
    case OIS::KC_T:
        mCurrentFullness = Tile::nextTileFullness((int)mCurrentFullness);
        updateCursorText();
        break;

    // Quit the Editor Mode
    case OIS::KC_ESCAPE:
        regressMode();
        break;

    case OIS::KC_F8:
        MapLoader::writeGameMapToFile(std::string("levels/Test.level") + ".out", *frameListener->getGameMap());
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

bool EditorMode::keyReleased(const OIS::KeyEvent& arg)
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

    case OIS::KC_Q:
        camMgr.move(camMgr.stopRotLeft);
        break;

    case OIS::KC_E:
        camMgr.move(camMgr.stopRotRight);
        break;

    case OIS::KC_PGUP:
        camMgr.move(camMgr.stopRotUp);
        break;

    case OIS::KC_PGDOWN:
        camMgr.move(camMgr.stopRotDown);
        break;

    case OIS::KC_HOME:
        camMgr.stopZooming();
        break;

    case OIS::KC_END:
        camMgr.stopZooming();
        break;

    default:
        break;
    }

    return true;
}

void EditorMode::handleHotkeys(OIS::KeyCode keycode)
{
    //keycode minus two because the codes are shifted by two against the actual number
    unsigned int keynumber = keycode - 2;

    CameraManager* cm = ODFrameListener::getSingletonPtr()->cm;
    InputManager* inputManager = mModeManager->getInputManager();

    if (getKeyboard()->isModifierDown(OIS::Keyboard::Shift))
    {
        inputManager->mHotkeyLocationIsValid[keynumber] = true;
        inputManager->mHotkeyLocation[keynumber] = cm->getCameraViewTarget();
    }
    else
    {
        if (inputManager->mHotkeyLocationIsValid[keynumber])
            cm->flyTo(inputManager->mHotkeyLocation[keynumber]);
    }
}

//! Rendering methods
void EditorMode::onFrameStarted(const Ogre::FrameEvent& evt)
{
    CameraManager* cm = ODFrameListener::getSingletonPtr()->cm;
    cm->moveCamera(evt.timeSinceLastFrame);

    MiniMap* minimap = ODFrameListener::getSingleton().getMiniMap();
    minimap->draw();
    minimap->swap();
}

void EditorMode::onFrameEnded(const Ogre::FrameEvent& evt)
{
}
