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
#include "ODClient.h"
#include "ODServer.h"
#include "ODApplication.h"

#include <OgreEntity.h>
#include <OgreRoot.h>
#include <OgreRenderWindow.h>

#include <algorithm>
#include <vector>
#include <string>
#include <cstdio>

EditorMode::EditorMode(ModeManager* modeManager):
    AbstractApplicationMode(modeManager, ModeManager::EDITOR),
    mCurrentTileType(Tile::TileType::nullTileType),
    mCurrentFullness(100.0),
    mCurrentSeatId(0),
    mGameMap(ODFrameListener::getSingletonPtr()->getClientGameMap()),
    mMouseX(0),
    mMouseY(0),
    mMouseLight(NULL)
{
    // Set per default the input on the map
    mModeManager->getInputManager()->mMouseDownOnCEGUIWindow = false;

    // Keep track of the mouse light object
    Ogre::SceneManager* sceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
    mMouseLight = sceneMgr->getLight("MouseLight");
}

EditorMode::~EditorMode()
{
}

void EditorMode::activate()
{
    // Loads the corresponding Gui sheet.
    Gui::getSingleton().loadGuiSheet(Gui::editorModeGui);

    MiniMap* minimap = ODFrameListener::getSingleton().getMiniMap();
    minimap->attachMiniMap(Gui::guiSheet::editorModeGui);

    giveFocus();

    // Stop the game music.
    MusicPlayer::getSingleton().stop();

    // By default, we set the current seat id to the connected player
    Player* player = mGameMap->getLocalPlayer();
    mCurrentSeatId = player->getSeat()->getId();

    mGameMap->setGamePaused(false);
}

bool EditorMode::mouseMoved(const OIS::MouseEvent &arg)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition((float)arg.state.X.abs, (float)arg.state.Y.abs);

    if (!isConnected())
        return true;

    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    InputManager* inputManager = mModeManager->getInputManager();

    if (frameListener->isTerminalActive())
        return true;

    Player* player = mGameMap->getLocalPlayer();
    Room::RoomType selectedRoomType = player->getNewRoomType();
    Trap::TrapType selectedTrapType = player->getNewTrapType();
    if (player->getNewRoomType() != Room::nullRoomType
        || player->getNewTrapType() != Trap::nullTrapType
        || mCurrentTileType != Tile::TileType::nullTileType)
    {
        TextRenderer::getSingleton().moveText(ODApplication::POINTER_INFO_STRING,
            (Ogre::Real)(arg.state.X.abs + 30), (Ogre::Real)arg.state.Y.abs);

        if(selectedRoomType != Room::nullRoomType)
        {
            TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, std::string(Room::getRoomNameFromRoomType(selectedRoomType)));
        }
        else if(selectedTrapType != Trap::nullTrapType)
        {
            TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, std::string(Trap::getTrapNameFromTrapType(selectedTrapType)));
        }
        else if(mCurrentTileType != Tile::TileType::nullTileType)
        {
            TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, std::string(Tile::tileTypeToString(mCurrentTileType)));
        }
    }

    handleMouseWheel(arg);

    handleCursorPositionUpdate();

    // Since this is a tile selection query we loop over the result set
    // and look for the first object which is actually a tile.
    Ogre::RaySceneQueryResult& result = ODFrameListener::getSingleton().doRaySceneQuery(arg);
    for (Ogre::RaySceneQueryResult::iterator itr = result.begin(); itr != result.end(); ++itr)
    {
        if (itr->movable == NULL)
            continue;

        // Check to see if the current query result is a tile.
        std::string resultName = itr->movable->getName();

        // Checks which tile we are on (if any)
        if (!Tile::checkTileName(resultName, inputManager->mXPos, inputManager->mYPos))
            continue;

        // If we don't drag anything, there is no affected tiles to compute.
        if (!inputManager->mLMouseDown || inputManager->mDragType == nullDragType)
            break;

        for (int jj = 0; jj < mGameMap->getMapSizeY(); ++jj)
        {
            for (int ii = 0; ii < mGameMap->getMapSizeX(); ++ii)
            {
                mGameMap->getTile(ii, jj)->setSelected(false, player);
            }
        }

        // Loop over the tiles in the rectangular selection region and set their setSelected flag accordingly.
        //TODO: This function is horribly inefficient, it should loop over a rectangle selecting tiles by x-y coords
        // rather than the reverse that it is doing now.
        std::vector<Tile*> affectedTiles = mGameMap->rectangularRegion(inputManager->mXPos,
                                                                        inputManager->mYPos,
                                                                        inputManager->mLStartDragX,
                                                                        inputManager->mLStartDragY);

        for( std::vector<Tile*>::iterator itr = affectedTiles.begin(); itr != affectedTiles.end(); ++itr)
        {
            (*itr)->setSelected(true, player);
        }

        break;
    }

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
        // Since this is a tile selection query we loop over the result set and look for the first object which is actually a tile.
        for (; itr != end; ++itr)
        {
            if (itr->movable == NULL)
                continue;

            // Check to see if the current query result is a tile.
            std::string resultName = itr->movable->getName();

            // Checks which tile we are on (if any)
            if (!Tile::checkTileName(resultName, inputManager->mXPos, inputManager->mYPos))
                continue;

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

    } // switch drag type
}

bool EditorMode::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
        Gui::getSingletonPtr()->convertButton(id));

    if (!isConnected())
        return true;

    CEGUI::Window *tempWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getWindowContainingMouse();

    InputManager* inputManager = mModeManager->getInputManager();

    // If the mouse press is on a CEGUI window ignore it
    if (tempWindow != NULL && tempWindow->getName().compare("EDITORGUI") != 0)
    {
        inputManager->mMouseDownOnCEGUIWindow = true;
        return true;
    }

    inputManager->mMouseDownOnCEGUIWindow = false;

    // There is a bug in OIS. When playing in windowed mode, if we clic outside the window
    // and then we restore the window, we will receive a clic event on the last place where
    // the mouse was.
    Ogre::RenderWindow* mainWindows = static_cast<Ogre::RenderWindow*>(
        Ogre::Root::getSingleton().getRenderTarget("OpenDungeons " + ODApplication::VERSION));
    if((!mainWindows->isFullScreen()) &&
       ((arg.state.X.abs == 0) || (arg.state.Y.abs == 0) ||
        (static_cast<Ogre::uint32>(arg.state.X.abs) == mainWindows->getWidth()) ||
        (static_cast<Ogre::uint32>(arg.state.Y.abs) == mainWindows->getHeight())))
    {
        return true;
    }

    Ogre::RaySceneQueryResult &result = ODFrameListener::getSingleton().doRaySceneQuery(arg);
    Ogre::RaySceneQueryResult::iterator itr = result.begin();

    if (id == OIS::MB_Middle)
    {
        // See if the mouse is over any creatures
        for (;itr != result.end(); ++itr)
        {
            if (itr->movable == NULL)
                continue;

            std::string resultName = itr->movable->getName();

            if (resultName.find(Creature::CREATURE_PREFIX) == std::string::npos)
                continue;

            Creature* tempCreature = mGameMap->getCreature(resultName.substr(
                Creature::CREATURE_PREFIX.length(), resultName.length()));

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
        mCurrentTileType = Tile::TileType::nullTileType;
        TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "");

        // If we right clicked with the mouse over a valid map tile, try to drop a creature onto the map.
        Tile *curTile = mGameMap->getTile(inputManager->mXPos, inputManager->mYPos);

        if (curTile == NULL)
            return true;

        if (mGameMap->getLocalPlayer()->isDropCreaturePossible(curTile, 0, true))
        {
            if(ODClient::getSingleton().isConnected())
            {
                // Send a message to the server telling it we want to drop the creature
                ClientNotification *clientNotification = new ClientNotification(
                    ClientNotification::askCreatureDrop);
                clientNotification->mPacket << curTile;
                ODClient::getSingleton().queueClientNotification(clientNotification);
            }

            return true;
        }
    }

    if (id != OIS::MB_Left)
        return true;

    // Left mouse button down
    inputManager->mLMouseDown = true;
    inputManager->mLStartDragX = inputManager->mXPos;
    inputManager->mLStartDragY = inputManager->mYPos;

    CameraManager* cm = ODFrameListener::getSingletonPtr()->cm;

    // Check whether the player is already placing rooms or traps.
    bool skipCreaturePickUp = false;
    Player* player = mGameMap->getLocalPlayer();
    if (player && (player->getNewRoomType() != Room::nullRoomType
        || player->getNewTrapType() != Trap::nullTrapType
        || mCurrentTileType != Tile::TileType::nullTileType))
    {
        skipCreaturePickUp = true;
    }

    // Check whether the player selection is over a wall and skip creature in that case
    // to permit easier wall selection.
    if (mGameMap->getTile(mMouseX, mMouseY)->getFullness() > 1.0)
        skipCreaturePickUp = true;

    // See if the mouse is over any creatures
    for (;itr != result.end(); ++itr)
    {
        // Skip picking up creatures when placing rooms or traps
        // as creatures often get in the way.
        if (skipCreaturePickUp)
            break;

        if (itr->movable == NULL)
            continue;

        std::string resultName = itr->movable->getName();

        if (resultName.find(Creature::CREATURE_PREFIX) == std::string::npos)
            continue;

        // Pick the creature up and put it in our hand
        if(inputManager->mExpectCreatureClick)
        {
            mModeManager->requestFppMode();
            const string& tmp_name =  (itr->movable->getName());
            std::cerr << tmp_name.substr(9, tmp_name.size()) << std::endl;
            cm->setFPPCamera(mGameMap->getCreature(tmp_name.substr(9, tmp_name.size())));
            cm->setActiveCameraNode("FPP");
            cm->setActiveCamera("FPP");

            inputManager->mExpectCreatureClick = false;
        }
        else
        {
            // The creature name is after the creature prefix
            std::string creatureName = resultName.substr(Creature::CREATURE_PREFIX.length());
            Creature* currentCreature = mGameMap->getCreature(creatureName);
            if (currentCreature == NULL)
                continue;

            // In editor mode, we allow pickup of all creatures. No need to test color
            if (ODClient::getSingleton().isConnected())
            {
                // Send a message to the server telling it we want to pick up this creature
                ClientNotification *clientNotification = new ClientNotification(
                    ClientNotification::askCreaturePickUp);
                std::string name = currentCreature->getName();
                clientNotification->mPacket << name;
                ODClient::getSingleton().queueClientNotification(clientNotification);
                return true;
            }
        }
    }

    // If no creatures or lights are under the  mouse run through the list again to check for tiles
    for (itr = result.begin(); itr != result.end(); ++itr)
    {
        if (itr->movable == NULL)
            continue;

        std::string resultName = itr->movable->getName();

        int x, y;
        if (!Tile::checkTileName(resultName, x, y))
            continue;

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

        // If we have selected a tile type, we use it
        else if (mCurrentTileType != Tile::TileType::nullTileType)
        {
            inputManager->mDragType = changeTile;
        }

        break;
    }

    return true;
}

bool EditorMode::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(Gui::getSingletonPtr()->convertButton(id));

    InputManager* inputManager = mModeManager->getInputManager();
    int dragType = inputManager->mDragType;
    inputManager->mDragType = nullDragType;

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

    if (id != OIS::MB_Left)
        return true;

    // Left mouse button up
    inputManager->mLMouseDown = false;

    switch(dragType)
    {
        default:
            dragType = nullDragType;
            return true;

        // When either selecting a tile, adding room or a trap
        // we do what's next.
        case addNewRoom:
        case addNewTrap:
        case changeTile:
            break;
    }

    // On the client:  Inform the server about our choice
    if(dragType == changeTile)
    {
        double fullness;
        switch(mCurrentTileType)
        {
            case Tile::TileType::nullTileType:
                return true;
            case Tile::TileType::dirt:
            case Tile::TileType::gold:
            case Tile::TileType::rock:
            case Tile::TileType::claimed:
                fullness = mCurrentFullness;
                break;
            default:
                fullness = 0.0;
        }
        int intTileType = static_cast<int>(mCurrentTileType);
        ClientNotification *clientNotification = new ClientNotification(
            ClientNotification::editorAskChangeTiles);
        clientNotification->mPacket << inputManager->mXPos << inputManager->mYPos;
        clientNotification->mPacket << inputManager->mLStartDragX << inputManager->mLStartDragY;
        clientNotification->mPacket << intTileType;
        clientNotification->mPacket << fullness;
        clientNotification->mPacket << mCurrentSeatId;
        ODClient::getSingleton().queueClientNotification(clientNotification);
    }
    else if(dragType == addNewRoom)
    {
        int intRoomType = static_cast<int>(mGameMap->getLocalPlayer()->getNewRoomType());
        ClientNotification *clientNotification = new ClientNotification(
            ClientNotification::editorAskBuildRoom);
        clientNotification->mPacket << inputManager->mXPos << inputManager->mYPos;
        clientNotification->mPacket << inputManager->mLStartDragX << inputManager->mLStartDragY;
        clientNotification->mPacket << intRoomType << mCurrentSeatId;
        ODClient::getSingleton().queueClientNotification(clientNotification);
    }
    else if(dragType == addNewTrap)
    {
        ClientNotification *clientNotification = new ClientNotification(
            ClientNotification::editorAskBuildTrap);
        int intTrapType = static_cast<int>(mGameMap->getLocalPlayer()->getNewTrapType());
        clientNotification->mPacket << inputManager->mXPos << inputManager->mYPos;
        clientNotification->mPacket << inputManager->mLStartDragX << inputManager->mLStartDragY;
        clientNotification->mPacket << intTrapType << mCurrentSeatId;
        ODClient::getSingleton().queueClientNotification(clientNotification);
    }
    return true;
}

void EditorMode::updateCursorText()
{
    // Gets the current action from the drag type
    std::stringstream textSS;

    // Update the fullness info
    CEGUI::Window *posWin = Gui::getSingletonPtr()->getGuiSheet(Gui::editorModeGui)->getChild(Gui::EDITOR_FULLNESS);
    textSS.str("");
    textSS << "Tile Fullness (T): " << mCurrentFullness << "%";
    posWin->setText(textSS.str());

    // Update the cursor position
    posWin = Gui::getSingletonPtr()->getGuiSheet(Gui::editorModeGui)->getChild(Gui::EDITOR_CURSOR_POS);
    textSS.str("");
    textSS << "Cursor: x: " << mMouseX << ", y: " << mMouseY;
    posWin->setText(textSS.str());

    // Update the seat id
    posWin = Gui::getSingletonPtr()->getGuiSheet(Gui::editorModeGui)->getChild(Gui::EDITOR_SEAT_ID);
    textSS.str("");
    textSS << "Seat id (Y): " << mCurrentSeatId;
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
        mCurrentTileType = Tile::nextTileType(mCurrentTileType);
        updateCursorText();
        break;

    //Toggle mCurrentFullness
    case OIS::KC_T:
        mCurrentFullness = Tile::nextTileFullness((int)mCurrentFullness);
        updateCursorText();
        break;

    //Toggle mCurrentSeatId
    case OIS::KC_Y:
        mCurrentSeatId = mGameMap->nextSeatId(mCurrentSeatId);
        updateCursorText();
        break;

    // Quit the Editor Mode
    case OIS::KC_ESCAPE:
        regressMode();
        break;

    case OIS::KC_F8:
        if(ODClient::getSingleton().isConnected())
        {
            // Send a message to the server telling it we want to drop the creature
            ClientNotification *clientNotification = new ClientNotification(
                ClientNotification::editorAskSaveMap);
            ODClient::getSingleton().queueClientNotification(clientNotification);
        }
        break;

    // Print a screenshot
    case OIS::KC_SYSRQ:
        ResourceManager::getSingleton().takeScreenshot(frameListener->getRenderWindow());
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

void EditorMode::exitMode()
{
    if(ODClient::getSingleton().isConnected())
        ODClient::getSingleton().disconnect();
    if(ODServer::getSingleton().isConnected())
        ODServer::getSingleton().stopServer();

    // Now that the server is stopped, we can clear the client game map
    // We process RenderRequests in case there is graphical things pending
    RenderManager::getSingleton().processRenderRequests();
    ODFrameListener::getSingleton().getClientGameMap()->clearAll();
    // We process again RenderRequests to destroy/delete what clearAll has put in the queue
    RenderManager::getSingleton().processRenderRequests();
}
