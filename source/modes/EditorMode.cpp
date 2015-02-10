/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
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

#include "modes/EditorMode.h"

#include "gamemap/MiniMap.h"
#include "gamemap/MapLoader.h"
#include "render/ODFrameListener.h"
#include "utils/LogManager.h"
#include "render/Gui.h"
#include "utils/ResourceManager.h"
#include "render/TextRenderer.h"
#include "entities/Creature.h"
#include "entities/MapLight.h"
#include "game/Seat.h"
#include "traps/Trap.h"
#include "game/Player.h"
#include "render/RenderManager.h"
#include "camera/CameraManager.h"
#include "rooms/Room.h"
#include "sound/MusicPlayer.h"
#include "network/ODClient.h"
#include "network/ODServer.h"
#include "ODApplication.h"
#include "entities/RenderedMovableEntity.h"

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
    mCurrentCreatureIndex(0),
    mGameMap(ODFrameListener::getSingletonPtr()->getClientGameMap()),
    mMouseX(0),
    mMouseY(0)
{
    // Set per default the input on the map
    mModeManager->getInputManager()->mMouseDownOnCEGUIWindow = false;
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

    InputManager* inputManager = mModeManager->getInputManager();

    Player* player = mGameMap->getLocalPlayer();
    Player::SelectedAction playerSelectedAction = player->getCurrentAction();
    if (playerSelectedAction != Player::SelectedAction::none)
    {
        TextRenderer::getSingleton().moveText(ODApplication::POINTER_INFO_STRING,
            static_cast<Ogre::Real>(arg.state.X.abs + 30), static_cast<Ogre::Real>(arg.state.Y.abs));

        switch(playerSelectedAction)
        {
            case Player::SelectedAction::buildRoom:
            {
                RoomType selectedRoomType = player->getNewRoomType();
                TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, std::string(Room::getRoomNameFromRoomType(selectedRoomType)));
                break;
            }
            case Player::SelectedAction::buildTrap:
            {
                Trap::TrapType selectedTrapType = player->getNewTrapType();
                TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, std::string(Trap::getTrapNameFromTrapType(selectedTrapType)));
                break;
            }
            case Player::SelectedAction::changeTile:
            {
                TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, std::string(Tile::tileTypeToString(mCurrentTileType)));
                break;
            }
            case Player::SelectedAction::destroyRoom:
            {
                TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "Destroy room");
                break;
            }
            case Player::SelectedAction::destroyTrap:
            {
                TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "Destroy trap");
                break;
            }
            default :
                break;
        }
    }

    handleMouseWheel(arg);

    handleCursorPositionUpdate();

    // Since this is a tile selection query we loop over the result set
    // and look for the first object which is actually a tile.
    Ogre::RaySceneQueryResult& result = ODFrameListener::getSingleton().doRaySceneQuery(arg);
    for (Ogre::RaySceneQueryResult::iterator itr = result.begin(); itr != result.end(); ++itr)
    {
        if (itr->movable == nullptr)
            continue;

        // Check to see if the current query result is a tile.
        std::string resultName = itr->movable->getName();

        // Checks which tile we are on (if any)
        if (!Tile::checkTileName(resultName, inputManager->mXPos, inputManager->mYPos))
            continue;

        // If we don't drag anything, there is no affected tiles to compute.
        if (!inputManager->mLMouseDown || player->getCurrentAction() == Player::SelectedAction::none)
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
    ODFrameListener& frameListener = ODFrameListener::getSingleton();

    if (arg.state.Z.rel > 0)
    {
        if (getKeyboard()->isModifierDown(OIS::Keyboard::Ctrl))
        {
            mGameMap->getLocalPlayer()->rotateHand(Player::Direction::left);
        }
        else
        {
            frameListener.moveCamera(CameraManager::moveDown);
        }
    }
    else if (arg.state.Z.rel < 0)
    {
        if (getKeyboard()->isModifierDown(OIS::Keyboard::Ctrl))
        {
            mGameMap->getLocalPlayer()->rotateHand(Player::Direction::right);
        }
        else
        {
            frameListener.moveCamera(CameraManager::moveUp);
        }
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
    Ogre::Real x = static_cast<Ogre::Real>(mMouseX);
    Ogre::Real y = static_cast<Ogre::Real>(mMouseY);
    RenderManager::getSingleton().moveCursor(x, y);

    updateCursorText();
}

void EditorMode::handleMouseMovedDragType(const OIS::MouseEvent &arg)
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    Ogre::RaySceneQueryResult& result = frameListener->doRaySceneQuery(arg);

    Ogre::RaySceneQueryResult::iterator itr = result.begin();
    Ogre::RaySceneQueryResult::iterator end = result.end();

    Player* player = mGameMap->getLocalPlayer();
    InputManager* inputManager = mModeManager->getInputManager();

    switch(player->getCurrentAction())
    {
    default:
        // Since this is a tile selection query we loop over the result set and look for the first object which is actually a tile.
        for (; itr != end; ++itr)
        {
            if (itr->movable == nullptr)
                continue;

            // Check to see if the current query result is a tile.
            std::string resultName = itr->movable->getName();

            // Checks which tile we are on (if any)
            if (!Tile::checkTileName(resultName, inputManager->mXPos, inputManager->mYPos))
                continue;

            handleCursorPositionUpdate();

            // If we don't drag anything, there is no affected tiles to compute.
            if (!inputManager->mLMouseDown || player->getCurrentAction() == Player::SelectedAction::none)
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
    if (tempWindow != nullptr && tempWindow->getName().compare("EDITORGUI") != 0)
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
            if (itr->movable == nullptr)
                continue;

            std::string resultName = itr->movable->getName();

            GameEntity* entity = getEntityFromOgreName(resultName);
            if (entity == nullptr || !entity->canDisplayStatsWindow(mGameMap->getLocalPlayer()->getSeat()))
                continue;

            entity->createStatsWindow();

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
        mGameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::none);
        mGameMap->getLocalPlayer()->setNewRoomType(RoomType::nullRoomType);
        mGameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
        mCurrentTileType = Tile::TileType::nullTileType;
        TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "");

        // If we right clicked with the mouse over a valid map tile, try to drop a creature onto the map.
        Tile *curTile = mGameMap->getTile(inputManager->mXPos, inputManager->mYPos);

        if (curTile == nullptr)
            return true;


        if(mGameMap->getLocalPlayer()->numObjectsInHand() > 0)
        {
            // If we right clicked with the mouse over a valid map tile, try to drop what we have in hand on the map.
            Tile *curTile = mGameMap->getTile(inputManager->mXPos, inputManager->mYPos);

            if (curTile == nullptr)
                return true;

            if (mGameMap->getLocalPlayer()->isDropHandPossible(curTile, 0))
            {
                if(ODClient::getSingleton().isConnected())
                {
                    // Send a message to the server telling it we want to drop the creature
                    ClientNotification *clientNotification = new ClientNotification(
                        ClientNotificationType::askHandDrop);
                    mGameMap->tileToPacket(clientNotification->mPacket, curTile);
                    ODClient::getSingleton().queueClientNotification(clientNotification);
                }

                return true;
            }
        }
        else
        {
            // No creature in hand. We check if we want to slap something
            for (;itr != result.end(); ++itr)
            {
                if (itr->movable == nullptr)
                    continue;

                std::string resultName = itr->movable->getName();

                GameEntity* entity = getEntityFromOgreName(resultName);
                if (entity == nullptr || !entity->canSlap(mGameMap->getLocalPlayer()->getSeat()))
                    continue;

                if(ODClient::getSingleton().isConnected())
                {
                    ODClient::getSingleton().queueClientNotification(ClientNotificationType::askSlapEntity,
                                                                     entity->getObjectType(),
                                                                     entity->getName());
                }

                return true;
            }
        }
    }

    if (id != OIS::MB_Left)
        return true;

    // Left mouse button down
    inputManager->mLMouseDown = true;
    inputManager->mLStartDragX = inputManager->mXPos;
    inputManager->mLStartDragY = inputManager->mYPos;

    // Check whether the player is already placing rooms or traps.
    bool skipCreaturePickUp = false;
    Player* player = mGameMap->getLocalPlayer();
    if (player && (player->getCurrentAction() != Player::SelectedAction::none))
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

        if (itr->movable == nullptr)
            continue;

        std::string resultName = itr->movable->getName();

        GameEntity* entity = getEntityFromOgreName(resultName);
        if (entity == nullptr || !entity->tryPickup(player->getSeat()))
            continue;

        if (ODClient::getSingleton().isConnected())
        {
            GameEntityType entityType = entity->getObjectType();
            const std::string& entityName = entity->getName();
            ClientNotification *clientNotification = new ClientNotification(
                ClientNotificationType::askEntityPickUp);
            clientNotification->mPacket << entityType;
            clientNotification->mPacket << entityName;
            ODClient::getSingleton().queueClientNotification(clientNotification);
        }
        return true;
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

    if (id != OIS::MB_Left)
        return true;

    // Left mouse button up
    inputManager->mLMouseDown = false;

    // On the client:  Inform the server about what we are doing
    switch(mGameMap->getLocalPlayer()->getCurrentAction())
    {
        case Player::SelectedAction::changeTile:
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
                    break;
            }
            int intTileType = static_cast<int>(mCurrentTileType);
            ClientNotification *clientNotification = new ClientNotification(
                ClientNotificationType::editorAskChangeTiles);
            clientNotification->mPacket << inputManager->mXPos << inputManager->mYPos;
            clientNotification->mPacket << inputManager->mLStartDragX << inputManager->mLStartDragY;
            clientNotification->mPacket << intTileType;
            clientNotification->mPacket << fullness;
            clientNotification->mPacket << mCurrentSeatId;
            ODClient::getSingleton().queueClientNotification(clientNotification);
            break;
        }
        case Player::SelectedAction::buildRoom:
        {
            int intRoomType = static_cast<int>(mGameMap->getLocalPlayer()->getNewRoomType());
            ClientNotification *clientNotification = new ClientNotification(
                ClientNotificationType::editorAskBuildRoom);
            clientNotification->mPacket << inputManager->mXPos << inputManager->mYPos;
            clientNotification->mPacket << inputManager->mLStartDragX << inputManager->mLStartDragY;
            clientNotification->mPacket << intRoomType << mCurrentSeatId;
            ODClient::getSingleton().queueClientNotification(clientNotification);
            break;
        }
        case Player::SelectedAction::buildTrap:
        {
            ClientNotification *clientNotification = new ClientNotification(
                ClientNotificationType::editorAskBuildTrap);
            int intTrapType = static_cast<int>(mGameMap->getLocalPlayer()->getNewTrapType());
            clientNotification->mPacket << inputManager->mXPos << inputManager->mYPos;
            clientNotification->mPacket << inputManager->mLStartDragX << inputManager->mLStartDragY;
            clientNotification->mPacket << intTrapType << mCurrentSeatId;
            ODClient::getSingleton().queueClientNotification(clientNotification);
            break;
        }
        case Player::SelectedAction::destroyRoom:
        {
            ClientNotification *clientNotification = new ClientNotification(
                ClientNotificationType::editorAskDestroyRoomTiles);
            clientNotification->mPacket << inputManager->mXPos << inputManager->mYPos;
            clientNotification->mPacket << inputManager->mLStartDragX << inputManager->mLStartDragY;
            ODClient::getSingleton().queueClientNotification(clientNotification);
            break;
        }
        case Player::SelectedAction::destroyTrap:
        {
            ClientNotification *clientNotification = new ClientNotification(
                ClientNotificationType::editorAskDestroyTrapTiles);
            clientNotification->mPacket << inputManager->mXPos << inputManager->mYPos;
            clientNotification->mPacket << inputManager->mLStartDragX << inputManager->mLStartDragY;
            ODClient::getSingleton().queueClientNotification(clientNotification);
            break;
        }
        default:
            return true;
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

    // Update the seat id
    posWin = Gui::getSingletonPtr()->getGuiSheet(Gui::editorModeGui)->getChild(Gui::EDITOR_CREATURE_SPAWN);
    textSS.str("");
    const CreatureDefinition* def = mGameMap->getClassDescription(mCurrentCreatureIndex);
    if(def == nullptr)
        textSS << "Creature (C): ?";
    else
        textSS << "Creature (C): " << def->getClassName();

    posWin->setText(textSS.str());
}

bool EditorMode::keyPressed(const OIS::KeyEvent &arg)
{
    ODFrameListener& frameListener = ODFrameListener::getSingleton();

    // Inject key to Gui
    CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyDown(static_cast<CEGUI::Key::Scan>(arg.key));
    CEGUI::System::getSingleton().getDefaultGUIContext().injectChar(arg.text);

    switch (arg.key)
    {
    case OIS::KC_F11:
        frameListener.toggleDebugInfo();
        break;

    case OIS::KC_GRAVE:
    case OIS::KC_F12:
        mModeManager->requestConsoleMode();
        break;

    case OIS::KC_LEFT:
    case OIS::KC_A:
        frameListener.moveCamera(CameraManager::Direction::moveLeft);
        break;

    case OIS::KC_RIGHT:
    case OIS::KC_D:
        frameListener.moveCamera(CameraManager::Direction::moveRight);
        break;

    case OIS::KC_UP:
    case OIS::KC_W:
        frameListener.moveCamera(CameraManager::Direction::moveForward);
        break;

    case OIS::KC_DOWN:
    case OIS::KC_S:
        frameListener.moveCamera(CameraManager::Direction::moveBackward);
        break;

    case OIS::KC_Q:
        frameListener.moveCamera(CameraManager::Direction::rotateLeft);
        break;

    case OIS::KC_E:
        frameListener.moveCamera(CameraManager::Direction::rotateRight);
        break;

    case OIS::KC_PGUP:
        frameListener.moveCamera(CameraManager::Direction::rotateUp);
        break;

    case OIS::KC_PGDOWN:
        frameListener.moveCamera(CameraManager::Direction::rotateDown);
        break;

    case OIS::KC_HOME:
        frameListener.moveCamera(CameraManager::Direction::moveUp);
        break;

    case OIS::KC_END:
        frameListener.moveCamera(CameraManager::Direction::moveDown);
        break;

    //Toggle mCurrentFullness
    case OIS::KC_T:
        mCurrentFullness = Tile::nextTileFullness(static_cast<int>(mCurrentFullness));
        updateCursorText();
        break;

    //Toggle mCurrentSeatId
    case OIS::KC_Y:
        mCurrentSeatId = mGameMap->nextSeatId(mCurrentSeatId);
        updateCursorText();
        break;

    //Toggle mCurrentCreatureIndex
    case OIS::KC_C:
        if(++mCurrentCreatureIndex >= mGameMap->numClassDescriptions())
        {
            mCurrentCreatureIndex = 0;
        }
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
                ClientNotificationType::editorAskSaveMap);
            ODClient::getSingleton().queueClientNotification(clientNotification);
        }
        break;

    // Print a screenshot
    case OIS::KC_SYSRQ:
        ResourceManager::getSingleton().takeScreenshot(frameListener.getRenderWindow());
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
    CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyUp(static_cast<CEGUI::Key::Scan>(arg.key));

    ODFrameListener& frameListener = ODFrameListener::getSingleton();

    switch (arg.key)
    {
    case OIS::KC_LEFT:
    case OIS::KC_A:
        frameListener.moveCamera(CameraManager::Direction::stopLeft);
        break;

    case OIS::KC_RIGHT:
    case OIS::KC_D:
        frameListener.moveCamera(CameraManager::Direction::stopRight);
        break;

    case OIS::KC_UP:
    case OIS::KC_W:
        frameListener.moveCamera(CameraManager::Direction::stopForward);
        break;

    case OIS::KC_DOWN:
    case OIS::KC_S:
        frameListener.moveCamera(CameraManager::Direction::stopBackward);
        break;

    case OIS::KC_Q:
        frameListener.moveCamera(CameraManager::Direction::stopRotLeft);
        break;

    case OIS::KC_E:
        frameListener.moveCamera(CameraManager::Direction::stopRotRight);
        break;

    case OIS::KC_PGUP:
        frameListener.moveCamera(CameraManager::Direction::stopRotUp);
        break;

    case OIS::KC_PGDOWN:
        frameListener.moveCamera(CameraManager::Direction::stopRotDown);
        break;

    case OIS::KC_HOME:
        frameListener.moveCamera(CameraManager::Direction::stopDown);
        break;

    case OIS::KC_END:
        frameListener.moveCamera(CameraManager::Direction::stopUp);
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

    ODFrameListener& frameListener = ODFrameListener::getSingleton();
    InputManager* inputManager = mModeManager->getInputManager();

    if (getKeyboard()->isModifierDown(OIS::Keyboard::Shift))
    {
        inputManager->mHotkeyLocationIsValid[keynumber] = true;
        inputManager->mHotkeyLocation[keynumber] = frameListener.getCameraViewTarget();
    }
    else
    {
        if (inputManager->mHotkeyLocationIsValid[keynumber])
            frameListener.cameraFlyTo(inputManager->mHotkeyLocation[keynumber]);
    }
}

//! Rendering methods
void EditorMode::onFrameStarted(const Ogre::FrameEvent& evt)
{
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
    ODFrameListener::getSingleton().getClientGameMap()->clearAll();
    ODFrameListener::getSingleton().getClientGameMap()->processDeletionQueues();
}

void EditorMode::notifyGuiAction(GuiAction guiAction)
{
    switch(guiAction)
    {
            case GuiAction::ButtonPressedCreatureWorker:
            {
                if(ODClient::getSingleton().isConnected())
                {
                    ClientNotification *clientNotification = new ClientNotification(
                        ClientNotificationType::editorCreateWorker);
                    clientNotification->mPacket << mCurrentSeatId;
                    ODClient::getSingleton().queueClientNotification(clientNotification);
                }
                break;
            }
            case GuiAction::ButtonPressedCreatureFighter:
            {
                if(ODClient::getSingleton().isConnected())
                {
                    const CreatureDefinition* def = mGameMap->getClassDescription(mCurrentCreatureIndex);
                    OD_ASSERT_TRUE(def != nullptr);
                    if(def == nullptr)
                        break;
                    ClientNotification *clientNotification = new ClientNotification(
                        ClientNotificationType::editorCreateFighter);
                    clientNotification->mPacket << mCurrentSeatId;
                    clientNotification->mPacket << def->getClassName();
                    ODClient::getSingleton().queueClientNotification(clientNotification);
                }
                break;
            }
            default:
                break;
    }
}
