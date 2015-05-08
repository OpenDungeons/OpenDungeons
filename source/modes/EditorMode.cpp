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

#include "gamemap/GameMap.h"
#include "gamemap/MiniMap.h"
#include "gamemap/MapLoader.h"
#include "render/ODFrameListener.h"
#include "render/Gui.h"
#include "render/TextRenderer.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/MapLight.h"
#include "entities/Tile.h"
#include "game/Seat.h"
#include "traps/TrapManager.h"
#include "game/Player.h"
#include "render/RenderManager.h"
#include "camera/CameraManager.h"
#include "rooms/RoomManager.h"
#include "sound/MusicPlayer.h"
#include "network/ODClient.h"
#include "network/ODServer.h"
#include "ODApplication.h"
#include "entities/RenderedMovableEntity.h"

#include "utils/LogManager.h"
#include "utils/ResourceManager.h"
#include "utils/Helper.h"

#include <OgreEntity.h>
#include <OgreRoot.h>
#include <OgreRenderWindow.h>

#include <CEGUI/widgets/PushButton.h>

#include <algorithm>
#include <vector>
#include <string>
#include <cstdio>

namespace
{
    //! \brief Functor to select tile type from gui
    class TileSelector
    {
    public:
        bool operator()(const CEGUI::EventArgs& e)
        {
            playerSelection.setCurrentAction(SelectedAction::changeTile);
            editorMode.setTileVisual(tileVisual);
            return true;
        }
        TileVisual tileVisual;
        PlayerSelection& playerSelection;
        EditorMode& editorMode;
    };
}

EditorMode::EditorMode(ModeManager* modeManager):
    GameEditorModeBase(modeManager, ModeManager::EDITOR, modeManager->getGui().getGuiSheet(Gui::guiSheet::editorModeGui)),
    mCurrentTileVisual(TileVisual::nullTileVisual),
    mCurrentFullness(100.0),
    mCurrentSeatId(0),
    mCurrentCreatureIndex(0),
    mMouseX(0),
    mMouseY(0)
{
    // Set per default the input on the map
    mModeManager->getInputManager()->mMouseDownOnCEGUIWindow = false;

    ODFrameListener::getSingleton().getCameraManager()->setDefaultView();

    // The Quit menu handlers
    addEventConnection(
        mRootWindow->getChild("ConfirmExit/__auto_closebutton__")->subscribeEvent(
            CEGUI::Window::EventMouseClick,
            CEGUI::Event::Subscriber(&EditorMode::hideQuitMenu, this)
    ));
    addEventConnection(
        mRootWindow->getChild("ConfirmExit/NoOption")->subscribeEvent(
            CEGUI::Window::EventMouseClick,
            CEGUI::Event::Subscriber(&EditorMode::hideQuitMenu, this)
    ));
    addEventConnection(
        mRootWindow->getChild("ConfirmExit/YesOption")->subscribeEvent(
            CEGUI::Window::EventMouseClick,
            CEGUI::Event::Subscriber(&EditorMode::onClickYesQuitMenu, this)
    ));

    // The options menu handlers
    addEventConnection(
        mRootWindow->getChild("OptionsButton")->subscribeEvent(
            CEGUI::Window::EventMouseClick,
            CEGUI::Event::Subscriber(&EditorMode::toggleOptionsWindow, this)
    ));
    addEventConnection(
        mRootWindow->getChild("EditorOptionsWindow/__auto_closebutton__")->subscribeEvent(
            CEGUI::Window::EventMouseClick,
            CEGUI::Event::Subscriber(&EditorMode::toggleOptionsWindow, this)
    ));
    addEventConnection(
        mRootWindow->getChild("EditorOptionsWindow/SaveLevelButton")->subscribeEvent(
            CEGUI::Window::EventMouseClick,
            CEGUI::Event::Subscriber(&EditorMode::onSaveButtonClickFromOptions, this)
    ));
    addEventConnection(
        mRootWindow->getChild("EditorOptionsWindow/QuitEditorButton")->subscribeEvent(
            CEGUI::Window::EventMouseClick,
            CEGUI::Event::Subscriber(&EditorMode::showQuitMenuFromOptions, this)
    ));

    //Map light
    connectGuiAction(Gui::EDITOR_MAPLIGHT_BUTTON,
                     AbstractApplicationMode::GuiAction::ButtonPressedMapLight);

    //Tile selection
    connectTileSelect(Gui::EDITOR_CLAIMED_BUTTON,TileVisual::claimedGround);
    connectTileSelect(Gui::EDITOR_DIRT_BUTTON,TileVisual::dirtGround);
    connectTileSelect(Gui::EDITOR_GOLD_BUTTON,TileVisual::goldGround);
    connectTileSelect(Gui::EDITOR_LAVA_BUTTON,TileVisual::lavaGround);
    connectTileSelect(Gui::EDITOR_ROCK_BUTTON,TileVisual::rockGround);
    connectTileSelect(Gui::EDITOR_WATER_BUTTON,TileVisual::waterGround);

    updateFlagColor();
}

EditorMode::~EditorMode()
{
    if(ODClient::getSingleton().isConnected())
        ODClient::getSingleton().disconnect();
    if(ODServer::getSingleton().isConnected())
        ODServer::getSingleton().stopServer();

    // Now that the server is stopped, we can clear the client game map
    ODFrameListener::getSingleton().getClientGameMap()->clearAll();
}

void EditorMode::activate()
{
    // Loads the corresponding Gui sheet.
    getModeManager().getGui().loadGuiSheet(Gui::editorModeGui);
    CEGUI::Window* guiSheet = mRootWindow;
    guiSheet->getChild("EditorOptionsWindow")->hide();
    guiSheet->getChild("ConfirmExit")->hide();
    // Hide also the Replay check-box as it doesn't make sense for the editor
    guiSheet->getChild("ConfirmExit/SaveReplayCheckbox")->hide();
    guiSheet->getChild("GameChatWindow/GameChatEditBox")->hide();

    giveFocus();

    // Stop the game music.
    MusicPlayer::getSingleton().stop();

    // By default, we set the current seat id to the connected player
    Player* player = mGameMap->getLocalPlayer();
    mCurrentSeatId = player->getSeat()->getId();

    refreshGuiResearch();

    mGameMap->setGamePaused(false);
}

bool EditorMode::mouseMoved(const OIS::MouseEvent &arg)
{
    AbstractApplicationMode::mouseMoved(arg);

    if (!isConnected())
        return true;

    InputManager* inputManager = mModeManager->getInputManager();

    Player* player = mGameMap->getLocalPlayer();
    SelectedAction playerSelectedAction = mPlayerSelection.getCurrentAction();
    if (playerSelectedAction != SelectedAction::none)
    {
        TextRenderer::getSingleton().moveText(ODApplication::POINTER_INFO_STRING,
            static_cast<Ogre::Real>(arg.state.X.abs + 30), static_cast<Ogre::Real>(arg.state.Y.abs));

        switch(playerSelectedAction)
        {
            case SelectedAction::buildRoom:
            {
                RoomType selectedRoomType = mPlayerSelection.getNewRoomType();
                TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, std::string(RoomManager::getRoomNameFromRoomType(selectedRoomType)));
                break;
            }
            case SelectedAction::buildTrap:
            {
                TrapType selectedTrapType = mPlayerSelection.getNewTrapType();
                TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, std::string(TrapManager::getTrapNameFromTrapType(selectedTrapType)));
                break;
            }
            case SelectedAction::changeTile:
            {
                TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, std::string(Tile::tileVisualToString(mCurrentTileVisual)));
                break;
            }
            case SelectedAction::destroyRoom:
            {
                TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "Destroy room");
                break;
            }
            case SelectedAction::destroyTrap:
            {
                TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "Destroy trap");
                break;
            }
            default :
                break;
        }
    }

    handleMouseWheel(arg);

    // Since this is a tile selection query we loop over the result set
    // and look for the first object which is actually a tile.
    Ogre::Vector3 keeperPos;
    Ogre::RaySceneQueryResult& result = ODFrameListener::getSingleton().doRaySceneQuery(arg, keeperPos);
    RenderManager::getSingleton().moveWorldCoords(keeperPos.x, keeperPos.y);

    for (Ogre::RaySceneQueryResult::iterator itr = result.begin(); itr != result.end(); ++itr)
    {
        if (itr->movable == nullptr)
            continue;

        // Check to see if the current query result is a tile.
        std::string resultName = itr->movable->getName();

        // Checks which tile we are on (if any)
        EntityBase* entity = getEntityFromOgreName(resultName);
        if((entity == nullptr) ||
           (entity->getObjectType() != GameEntityType::tile))
        {
            continue;
        }
        Tile* tile = static_cast<Tile*>(entity);
        inputManager->mXPos = tile->getX();
        inputManager->mYPos = tile->getY();
        if (mMouseX != inputManager->mXPos || mMouseY != inputManager->mYPos)
        {
            mMouseX = inputManager->mXPos;
            mMouseY = inputManager->mYPos;
            RenderManager::getSingleton().setHoveredTile(mMouseX, mMouseY);
            updateCursorText();
        }

        // If we don't drag anything, there is no affected tiles to compute.
        if (!inputManager->mLMouseDown || mPlayerSelection.getCurrentAction() == SelectedAction::none)
            break;

        for (int jj = 0; jj < mGameMap->getMapSizeY(); ++jj)
        {
            for (int ii = 0; ii < mGameMap->getMapSizeX(); ++ii)
            {
                mGameMap->getTile(ii, jj)->setSelected(false, player);
            }
        }

        // Loop over the tiles in the rectangular selection region and set their setSelected flag accordingly.
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

bool EditorMode::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
        Gui::convertButton(id));

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

            EntityBase* entity = getEntityFromOgreName(resultName);
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
        mPlayerSelection.setCurrentAction(SelectedAction::none);
        mCurrentTileVisual = TileVisual::nullTileVisual;
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

                EntityBase* entity = getEntityFromOgreName(resultName);
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
    Player* player = mGameMap->getLocalPlayer();
    if (mPlayerSelection.getCurrentAction() != SelectedAction::none)
    {
        // Skip picking up creatures when placing rooms or traps
        // as creatures often get in the way.
        return true;
    }

    // See if the mouse is over any creatures
    for (;itr != result.end(); ++itr)
    {
        if (itr->movable == nullptr)
            continue;

        std::string resultName = itr->movable->getName();

        EntityBase* entity = getEntityFromOgreName(resultName);
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
    CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(Gui::convertButton(id));

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
    switch(mPlayerSelection.getCurrentAction())
    {
        case SelectedAction::changeTile:
        {
            TileType tileType = TileType::nullTileType;
            int seatId = -1;
            double fullness = mCurrentFullness;
            switch(mCurrentTileVisual)
            {
                case TileVisual::nullTileVisual:
                    return true;
                case TileVisual::dirtGround:
                    tileType = TileType::dirt;
                    break;
                case TileVisual::goldGround:
                    tileType = TileType::gold;
                    break;
                case TileVisual::rockGround:
                    tileType = TileType::rock;
                    break;
                case TileVisual::claimedGround:
                case TileVisual::claimedFull:
                    tileType = TileType::dirt;
                    seatId = mCurrentSeatId;
                    break;
                case TileVisual::waterGround:
                    tileType = TileType::water;
                    fullness = 0.0;
                    break;
                case TileVisual::lavaGround:
                    tileType = TileType::lava;
                    fullness = 0.0;
                    break;
                default:
                    return true;
            }
            ClientNotification *clientNotification = new ClientNotification(
                ClientNotificationType::editorAskChangeTiles);
            clientNotification->mPacket << inputManager->mXPos << inputManager->mYPos;
            clientNotification->mPacket << inputManager->mLStartDragX << inputManager->mLStartDragY;
            clientNotification->mPacket << tileType;
            clientNotification->mPacket << fullness;
            clientNotification->mPacket << seatId;
            ODClient::getSingleton().queueClientNotification(clientNotification);
            break;
        }
        case SelectedAction::buildRoom:
        {
            int intRoomType = static_cast<int>(mPlayerSelection.getNewRoomType());
            ClientNotification *clientNotification = new ClientNotification(
                ClientNotificationType::editorAskBuildRoom);
            clientNotification->mPacket << inputManager->mXPos << inputManager->mYPos;
            clientNotification->mPacket << inputManager->mLStartDragX << inputManager->mLStartDragY;
            clientNotification->mPacket << intRoomType << mCurrentSeatId;
            ODClient::getSingleton().queueClientNotification(clientNotification);
            break;
        }
        case SelectedAction::buildTrap:
        {
            ClientNotification *clientNotification = new ClientNotification(
                ClientNotificationType::editorAskBuildTrap);
            int intTrapType = static_cast<int>(mPlayerSelection.getNewTrapType());
            clientNotification->mPacket << inputManager->mXPos << inputManager->mYPos;
            clientNotification->mPacket << inputManager->mLStartDragX << inputManager->mLStartDragY;
            clientNotification->mPacket << intTrapType << mCurrentSeatId;
            ODClient::getSingleton().queueClientNotification(clientNotification);
            break;
        }
        case SelectedAction::destroyRoom:
        {
            ClientNotification *clientNotification = new ClientNotification(
                ClientNotificationType::editorAskDestroyRoomTiles);
            clientNotification->mPacket << inputManager->mXPos << inputManager->mYPos;
            clientNotification->mPacket << inputManager->mLStartDragX << inputManager->mLStartDragY;
            ODClient::getSingleton().queueClientNotification(clientNotification);
            break;
        }
        case SelectedAction::destroyTrap:
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
    CEGUI::Window *posWin = mRootWindow->getChild(Gui::EDITOR_FULLNESS);
    textSS.str("");
    textSS << "Tile Fullness (T): " << mCurrentFullness << "%";
    posWin->setText(textSS.str());

    // Update the cursor position
    posWin = mRootWindow->getChild(Gui::EDITOR_CURSOR_POS);
    textSS.str("");
    textSS << "Cursor: x: " << mMouseX << ", y: " << mMouseY;
    posWin->setText(textSS.str());

    // Update the seat id
    posWin = mRootWindow->getChild(Gui::EDITOR_SEAT_ID);
    textSS.str("");
    textSS << "Seat id (Y): " << mCurrentSeatId;
    posWin->setText(textSS.str());

    // Update the seat id
    posWin = mRootWindow->getChild(Gui::EDITOR_CREATURE_SPAWN);
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
    case OIS::KC_F5:
        onSaveButtonClickFromOptions();
        break;

    case OIS::KC_F10:
        toggleOptionsWindow();
        break;

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
        updateFlagColor();
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
        showQuitMenu();
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
    GameEditorModeBase::onFrameStarted(evt);

    //Update the minimap
    CameraManager& cameraManager = *ODFrameListener::getSingleton().getCameraManager();
    mMiniMap.updateCameraInfo(cameraManager.getCameraViewTarget(),
                              cameraManager.getActiveCameraNode()->getOrientation().getRoll().valueRadians());
    mMiniMap.draw(*mGameMap);
    mMiniMap.swap();
}

void EditorMode::onFrameEnded(const Ogre::FrameEvent& evt)
{
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
            case GuiAction::ButtonPressedMapLight:
            {
                if(ODClient::getSingleton().isConnected())
                {
                    ClientNotification *clientNotification = new ClientNotification(
                        ClientNotificationType::editorAskCreateMapLight);
                    ODClient::getSingleton().queueClientNotification(clientNotification);
                }
                break;
            }
            default:
                break;
    }
}

bool EditorMode::toggleOptionsWindow(const CEGUI::EventArgs& /*arg*/)
{
    CEGUI::Window* options = mRootWindow->getChild("EditorOptionsWindow");
    if (options == nullptr)
        return true;

    if (options->isVisible())
        options->hide();
    else
        options->show();
    return true;
}

bool EditorMode::showQuitMenuFromOptions(const CEGUI::EventArgs& /*arg*/)
{
    CEGUI::Window* guiSheet = mRootWindow;
    guiSheet->getChild("ConfirmExit")->show();
    guiSheet->getChild("EditorOptionsWindow")->hide();
    return true;
}

bool EditorMode::onSaveButtonClickFromOptions(const CEGUI::EventArgs& /*arg*/)
{
    if(ODClient::getSingleton().isConnected())
    {
        // Send a message to the server telling it we want to drop the creature
        ClientNotification *clientNotification = new ClientNotification(
            ClientNotificationType::askSaveMap);
        ODClient::getSingleton().queueClientNotification(clientNotification);
    }
    return true;
}

bool EditorMode::showQuitMenu(const CEGUI::EventArgs& /*arg*/)
{
    mRootWindow->getChild("ConfirmExit")->show();
    return true;
}

bool EditorMode::hideQuitMenu(const CEGUI::EventArgs& /*arg*/)
{
    mRootWindow->getChild("ConfirmExit")->hide();
    return true;
}

bool EditorMode::onClickYesQuitMenu(const CEGUI::EventArgs& /*arg*/)
{
    //TODO: Test whether the level was modified and ask accordingly.
    regressMode();
    return true;
}

void EditorMode::refreshGuiResearch()
{
    // We show/hide the icons depending on available researches
    CEGUI::Window* guiSheet = mRootWindow;
    guiSheet->getChild(Gui::BUTTON_DORMITORY)->show();
    guiSheet->getChild(Gui::BUTTON_TREASURY)->show();
    guiSheet->getChild(Gui::BUTTON_HATCHERY)->show();
    guiSheet->getChild(Gui::BUTTON_LIBRARY)->show();
    guiSheet->getChild(Gui::BUTTON_CRYPT)->show();
    guiSheet->getChild(Gui::BUTTON_TRAININGHALL)->show();
    guiSheet->getChild(Gui::BUTTON_WORKSHOP)->show();
    guiSheet->getChild(Gui::BUTTON_TRAP_CANNON)->show();
    guiSheet->getChild(Gui::BUTTON_TRAP_SPIKE)->show();
    guiSheet->getChild(Gui::BUTTON_TRAP_BOULDER)->show();
    guiSheet->getChild(Gui::BUTTON_SPELL_SUMMON_WORKER)->show();
    guiSheet->getChild(Gui::BUTTON_SPELL_CALLTOWAR)->show();
    guiSheet->getChild(Gui::BUTTON_SPELL_CREATURE_HEAL)->show();
    guiSheet->getChild(Gui::BUTTON_SPELL_CREATURE_EXPLOSION)->show();

    // We also display the editor only buttons
    guiSheet->getChild(Gui::BUTTON_TEMPLE)->show();
    guiSheet->getChild(Gui::BUTTON_PORTAL)->show();
}

void EditorMode::connectTileSelect(const std::string& buttonName, TileVisual tileVisual)
{
    addEventConnection(
        mRootWindow->getChild(buttonName)->subscribeEvent(
          CEGUI::PushButton::EventClicked,
          CEGUI::Event::Subscriber(TileSelector{tileVisual, mPlayerSelection, *this})
        )
    );
}

void EditorMode::updateFlagColor()
{
    std::string colorStr = Helper::getImageColoursStringFromColourValue(mGameMap->getSeatById(mCurrentSeatId)->getColorValue());
    mRootWindow->getChild("HorizontalPipe/SeatIdDisplay/Icon")->setProperty("ImageColours", colorStr);
}
