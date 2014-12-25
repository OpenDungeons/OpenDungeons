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

#include "modes/GameMode.h"

#include "network/ODClient.h"
#include "render/Gui.h"
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
#include "modes/Console.h"
#include "sound/MusicPlayer.h"
#include "network/ODServer.h"
#include "ODApplication.h"
#include "entities/RenderedMovableEntity.h"
#include "gamemap/MiniMap.h"

#include <CEGUI/WindowManager.h>
#include <CEGUI/widgets/PushButton.h>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>

#include <algorithm>
#include <vector>
#include <string>

//! \brief Colors used by the room/trap text overlay
static Ogre::ColourValue white = Ogre::ColourValue(1.0f, 1.0f, 1.0f, 1.0f);
static Ogre::ColourValue red = Ogre::ColourValue(1.0f, 0.0f, 0.0, 1.0f);

GameMode::GameMode(ModeManager *modeManager):
    AbstractApplicationMode(modeManager, ModeManager::GAME),
    mDigSetBool(false),
    mGameMap(ODFrameListener::getSingletonPtr()->getClientGameMap()),
    mMouseX(0),
    mMouseY(0),
    mCurrentInputMode(InputModeNormal),
    mHelpWindow(nullptr)
{
    // Set per default the input on the map
    mModeManager->getInputManager()->mMouseDownOnCEGUIWindow = false;
}

GameMode::~GameMode()
{
    if (mHelpWindow != nullptr)
        mHelpWindow->destroy();
}

//! \brief Converts an int value into a 2 digits-long Hex string value.
std::string intTo2Hex(int i)
{
  std::stringstream stream;
  stream << std::setfill('0') << std::setw(2) << std::hex << i;
  return stream.str();
}

//! \brief Gets the CEGUI ImageColours string property (AARRGGBB format) corresponding
//! to the given Ogre ColourValue.
std::string getImageColoursStringFromColourValue(const Ogre::ColourValue& color)
{
    std::string colourStr = intTo2Hex(static_cast<int>(color.a * 255.0f)) + intTo2Hex(static_cast<int>(color.r * 255.0f))
        + intTo2Hex(static_cast<int>(color.g * 255.0f)) + intTo2Hex(static_cast<int>(color.b * 255.0f));
    std::string imageColours = "tl:" + colourStr + " tr:" + colourStr + " bl:" + colourStr + " br:" + colourStr;
    return imageColours;
}

void GameMode::activate()
{
    // Loads the corresponding Gui sheet.
    Gui::getSingleton().loadGuiSheet(Gui::inGameMenu);

    Gui::getSingleton().getGuiSheet(Gui::inGameMenu)->getChild(Gui::EXIT_CONFIRMATION_POPUP)->hide();

    MiniMap* minimap = ODFrameListener::getSingleton().getMiniMap();
    minimap->attachMiniMap(Gui::guiSheet::inGameMenu);

    giveFocus();

    // Play the game music.
    MusicPlayer::getSingleton().play(mGameMap->getLevelMusicFile()); // in game music

    // Show the player seat color on the horizontal pipe - AARRGGBB format
    // ex: "tl:FF0000FF tr:FF0000FF bl:FF0000FF br:FF0000FF"
    std::string colorStr = getImageColoursStringFromColourValue(mGameMap->getLocalPlayer()->getSeat()->getColorValue());
    Gui::getSingleton().getGuiSheet(Gui::inGameMenu)->getChild("HorizontalPipe")->setProperty("ImageColours", colorStr);

    if(mGameMap->getTurnNumber() != -1)
    {
        /* The game has been resumed from another mode (like console).
           Let's refresh the exit popup */
        popupExit(mGameMap->getGamePaused());
    }
    else
    {
        mGameMap->setGamePaused(false);
    }
}

void GameMode::handleCursorPositionUpdate()
{
    InputManager* inputManager = mModeManager->getInputManager();

    // Don't update if we didn't actually change the tile coordinate.
    if (mMouseX == inputManager->mXPos && mMouseY == inputManager->mYPos)
        return;

    // Updates mouse position for other functions.
    mMouseX = inputManager->mXPos;
    mMouseY = inputManager->mYPos;

    Ogre::Real x = static_cast<Ogre::Real>(mMouseX);
    Ogre::Real y = static_cast<Ogre::Real>(mMouseY);
    RenderManager::getSingleton().moveCursor(x, y);
}

bool GameMode::mouseMoved(const OIS::MouseEvent &arg)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition((float)arg.state.X.abs, (float)arg.state.Y.abs);

    if (!isConnected())
        return true;

    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    InputManager* inputManager = mModeManager->getInputManager();

    if (frameListener->isTerminalActive())
        return true;

    // If we have a room or trap (or later spell) selected, show what we have selected
    // TODO: This should be changed, or combined with an icon or something later.
    Player* player = mGameMap->getLocalPlayer();
    if (player->getCurrentAction() != Player::SelectedAction::none)
    {
        TextRenderer& textRenderer = TextRenderer::getSingleton();
        textRenderer.moveText(ODApplication::POINTER_INFO_STRING,
            (Ogre::Real)(arg.state.X.abs + 30), (Ogre::Real)arg.state.Y.abs);

        switch(player->getCurrentAction())
        {
            case Player::SelectedAction::buildRoom:
            {
                int nbTile = 1;
                // If the player is dragging to build, we display the total price the room/trap will cost.
                // If he is not, we display the price for 1 tile.
                if(inputManager->mLMouseDown)
                {
                    std::vector<Tile*> buildableTiles = mGameMap->getBuildableTilesForPlayerInArea(inputManager->mXPos,
                        inputManager->mYPos, inputManager->mLStartDragX, inputManager->mLStartDragY, player);
                    nbTile = buildableTiles.size();
                }

                int gold = player->getSeat()->getGold();
                Room::RoomType selectedRoomType = player->getNewRoomType();
                int price = Room::costPerTile(selectedRoomType) * nbTile;

                // Check whether the room type is the first treasury tile.
                // In that case, the cost of the first tile is 0, to prevent the player from being stuck
                // with no means to earn money.
                if (selectedRoomType == Room::treasury && mGameMap->numRoomsByTypeAndSeat(Room::treasury, player->getSeat()) == 0)
                    price -= Room::costPerTile(selectedRoomType);

                Ogre::ColourValue& textColor = (gold < price) ? red : white;
                textRenderer.setColor(ODApplication::POINTER_INFO_STRING, textColor);
                textRenderer.setText(ODApplication::POINTER_INFO_STRING, std::string(Room::getRoomNameFromRoomType(selectedRoomType))
                    + " [" + Ogre::StringConverter::toString(price)+ "]");
                break;
            }
            case Player::SelectedAction::buildTrap:
            {
                int nbTile = 1;
                // If the player is dragging to build, we display the total price the room/trap will cost.
                // If he is not, we display the price for 1 tile.
                if(inputManager->mLMouseDown)
                {
                    std::vector<Tile*> buildableTiles = mGameMap->getBuildableTilesForPlayerInArea(inputManager->mXPos,
                        inputManager->mYPos, inputManager->mLStartDragX, inputManager->mLStartDragY, player);
                    nbTile = buildableTiles.size();
                }

                int gold = player->getSeat()->getGold();
                Trap::TrapType selectedTrapType = player->getNewTrapType();
                int price = Trap::costPerTile(selectedTrapType) * nbTile;
                Ogre::ColourValue& textColor = (gold < price) ? red : white;
                textRenderer.setColor(ODApplication::POINTER_INFO_STRING, textColor);
                textRenderer.setText(ODApplication::POINTER_INFO_STRING, std::string(Trap::getTrapNameFromTrapType(selectedTrapType))
                    + " [" + Ogre::StringConverter::toString(price)+ "]");
                break;
            }
            case Player::SelectedAction::destroyRoom:
            {
                int goldRetrieved = 0;
                std::vector<Tile*> tiles;
                if(inputManager->mLMouseDown)
                {
                    tiles = mGameMap->rectangularRegion(inputManager->mXPos,
                        inputManager->mYPos, inputManager->mLStartDragX, inputManager->mLStartDragY);
                }
                else
                {
                    Tile* tile = mGameMap->getTile(inputManager->mXPos, inputManager->mYPos);
                    if(tile != NULL)
                        tiles.push_back(tile);
                }

                for(Tile* tile : tiles)
                {
                    if(tile->getCoveringRoom() == nullptr)
                        continue;

                    Room* room = tile->getCoveringRoom();
                    if(!room->getSeat()->canRoomBeDestroyedBy(player->getSeat()))
                        continue;

                    goldRetrieved += Room::costPerTile(room->getType()) / 2;
                }
                textRenderer.setColor(ODApplication::POINTER_INFO_STRING, white);
                textRenderer.setText(ODApplication::POINTER_INFO_STRING, "Destroy room ["
                    + Ogre::StringConverter::toString(goldRetrieved)+ "]");
                break;
            }
            case Player::SelectedAction::destroyTrap:
            {
                int goldRetrieved = 0;
                std::vector<Tile*> tiles;
                if(inputManager->mLMouseDown)
                {
                    tiles = mGameMap->rectangularRegion(inputManager->mXPos,
                        inputManager->mYPos, inputManager->mLStartDragX, inputManager->mLStartDragY);
                }
                else
                {
                    Tile* tile = mGameMap->getTile(inputManager->mXPos, inputManager->mYPos);
                    if(tile != NULL)
                        tiles.push_back(tile);
                }

                for(Tile* tile : tiles)
                {
                    if(tile->getCoveringTrap() == nullptr)
                        continue;

                    Trap* trap = tile->getCoveringTrap();
                    if(!trap->getSeat()->canTrapBeDestroyedBy(player->getSeat()))
                        continue;

                    goldRetrieved += Trap::costPerTile(trap->getType()) / 2;
                }
                textRenderer.setColor(ODApplication::POINTER_INFO_STRING, white);
                textRenderer.setText(ODApplication::POINTER_INFO_STRING, "Destroy trap ["
                    + Ogre::StringConverter::toString(goldRetrieved)+ "]");
                break;
            }
            default:
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
        if (itr->movable == NULL)
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

void GameMode::handleMouseWheel(const OIS::MouseEvent& arg)
{
    ODFrameListener& frameListener = ODFrameListener::getSingleton();

    if (arg.state.Z.rel > 0)
    {
        if (getKeyboard()->isModifierDown(OIS::Keyboard::Ctrl))
        {
            mGameMap->getLocalPlayer()->rotateHand(1);
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
            mGameMap->getLocalPlayer()->rotateHand(-1);
        }
        else
        {
            frameListener.moveCamera(CameraManager::moveUp);
        }
    }
    else
    {
        frameListener.cameraStopZoom();
    }
}

bool GameMode::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
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

    if(mGameMap->getGamePaused())
        return true;

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
        mGameMap->getLocalPlayer()->setNewRoomType(Room::nullRoomType);
        mGameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
        TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "");

        if(mGameMap->getLocalPlayer()->numObjectsInHand() > 0)
        {
            // If we right clicked with the mouse over a valid map tile, try to drop what we have in hand on the map.
            Tile *curTile = mGameMap->getTile(inputManager->mXPos, inputManager->mYPos);

            if (curTile == NULL)
                return true;

            if (mGameMap->getLocalPlayer()->isDropHandPossible(curTile))
            {
                if(ODClient::getSingleton().isConnected())
                {
                    // Send a message to the server telling it we want to drop the creature
                    ClientNotification *clientNotification = new ClientNotification(
                        ClientNotification::askHandDrop);
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
                if (itr->movable == NULL)
                    continue;

                std::string resultName = itr->movable->getName();

                GameEntity* entity = getEntityFromOgreName(resultName);
                if (entity == nullptr || !entity->canSlap(mGameMap->getLocalPlayer()->getSeat(), false))
                    continue;

                if(ODClient::getSingleton().isConnected())
                {
                    const std::string& entityName = entity->getName();
                    GameEntity::ObjectType entityType = entity->getObjectType();
                    ClientNotification *clientNotification = new ClientNotification(
                        ClientNotification::askSlapEntity);
                    clientNotification->mPacket << entityType << entityName;
                    ODClient::getSingleton().queueClientNotification(clientNotification);
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
    bool skipPickUp = false;
    Player* player = mGameMap->getLocalPlayer();
    if (player && (player->getCurrentAction() != Player::SelectedAction::none))
    {
        skipPickUp = true;
    }

    // Check whether the player selection is over a wall and skip creature in that case
    // to permit easier wall selection.
    if (mGameMap->getTile(mMouseX, mMouseY)->getFullness() > 1.0)
        skipPickUp = true;

    // See if the mouse is over any creatures
    for (;itr != result.end(); ++itr)
    {
        // Skip picking up creatures when placing rooms or traps
        // as creatures often get in the way.
        if (skipPickUp)
            break;

        if (itr->movable == NULL)
            continue;

        std::string resultName = itr->movable->getName();

        GameEntity* entity = getEntityFromOgreName(resultName);
        if (entity == nullptr || !entity->tryPickup(player->getSeat(), false))
            continue;

        if (ODClient::getSingleton().isConnected())
        {
            GameEntity::ObjectType entityType = entity->getObjectType();
            const std::string& entityName = entity->getName();
            ClientNotification *clientNotification = new ClientNotification(
                ClientNotification::askEntityPickUp);
            clientNotification->mPacket << entityType;
            clientNotification->mPacket << entityName;
            ODClient::getSingleton().queueClientNotification(clientNotification);
        }
        return true;
    }

    // If we are doing nothing and we click on a tile, it is a tile selection
    if(player->getCurrentAction() == Player::SelectedAction::none)
    {
        for (itr = result.begin(); itr != result.end(); ++itr)
        {
            if (itr->movable == NULL)
                continue;

            std::string resultName = itr->movable->getName();

            int x, y;
            if (!Tile::checkTileName(resultName, x, y))
                continue;

            player->setCurrentAction(Player::SelectedAction::selectTile);
            break;
        }
    }

    // If we are in a game we store the opposite of whether this tile is marked for digging or not, this allows us to mark tiles
    // by dragging out a selection starting from an unmarcked tile, or unmark them by starting the drag from a marked one.
    Tile *tempTile = mGameMap->getTile(inputManager->mXPos, inputManager->mYPos);

    if (tempTile != NULL)
        mDigSetBool = !(tempTile->getMarkedForDigging(mGameMap->getLocalPlayer()));

    return true;
}

bool GameMode::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(Gui::getSingletonPtr()->convertButton(id));

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

    // On the client:  Inform the server about what we are doing
    switch(mGameMap->getLocalPlayer()->getCurrentAction())
    {
        case Player::SelectedAction::selectTile:
        {
            ClientNotification *clientNotification = new ClientNotification(
                ClientNotification::askMarkTile);
            clientNotification->mPacket << inputManager->mXPos << inputManager->mYPos;
            clientNotification->mPacket << inputManager->mLStartDragX << inputManager->mLStartDragY;
            clientNotification->mPacket << mDigSetBool;
            ODClient::getSingleton().queueClientNotification(clientNotification);
            mGameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::none);

            // We mark the selected tiles
            std::vector<Tile*> tiles = mGameMap->getDiggableTilesForPlayerInArea(inputManager->mXPos,
                inputManager->mYPos, inputManager->mLStartDragX, inputManager->mLStartDragY,
                mGameMap->getLocalPlayer());
            if(!tiles.empty())
            {
                mGameMap->markTilesForPlayer(tiles, mDigSetBool, mGameMap->getLocalPlayer());
                SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::DIGSELECT);
                for(Tile* tile : tiles)
                    tile->refreshMesh();
            }
            break;
        }
        case Player::SelectedAction::buildRoom:
        {
            int intRoomType = static_cast<int>(mGameMap->getLocalPlayer()->getNewRoomType());
            ClientNotification *clientNotification = new ClientNotification(
                ClientNotification::askBuildRoom);
            clientNotification->mPacket << inputManager->mXPos << inputManager->mYPos;
            clientNotification->mPacket << inputManager->mLStartDragX << inputManager->mLStartDragY;
            clientNotification->mPacket << intRoomType;
            ODClient::getSingleton().queueClientNotification(clientNotification);
            break;
        }
        case Player::SelectedAction::buildTrap:
        {
            ClientNotification *clientNotification = new ClientNotification(
                ClientNotification::askBuildTrap);
            int intTrapType = static_cast<int>(mGameMap->getLocalPlayer()->getNewTrapType());
            clientNotification->mPacket << inputManager->mXPos << inputManager->mYPos;
            clientNotification->mPacket << inputManager->mLStartDragX << inputManager->mLStartDragY;
            clientNotification->mPacket << intTrapType;
            ODClient::getSingleton().queueClientNotification(clientNotification);
            break;
        }
        case Player::SelectedAction::destroyRoom:
        {
            ClientNotification *clientNotification = new ClientNotification(
                ClientNotification::askSellRoomTiles);
            clientNotification->mPacket << inputManager->mXPos << inputManager->mYPos;
            clientNotification->mPacket << inputManager->mLStartDragX << inputManager->mLStartDragY;
            ODClient::getSingleton().queueClientNotification(clientNotification);
            break;
        }
        case Player::SelectedAction::destroyTrap:
        {
            ClientNotification *clientNotification = new ClientNotification(
                ClientNotification::askSellTrapTiles);
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

bool GameMode::keyPressed(const OIS::KeyEvent &arg)
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    if (frameListener->isTerminalActive())
        return true;

    // Inject key to Gui
    CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyDown((CEGUI::Key::Scan) arg.key);
    CEGUI::System::getSingleton().getDefaultGUIContext().injectChar(arg.text);

    if((mCurrentInputMode == InputModeChat) && isChatKey(arg))
        return keyPressedChat(arg);

    return keyPressedNormal(arg);
}

bool GameMode::keyPressedNormal(const OIS::KeyEvent &arg)
{
    ODFrameListener& frameListener = ODFrameListener::getSingleton();
    InputManager* inputManager = mModeManager->getInputManager();

    switch (arg.key)
    {
    case OIS::KC_F1:
        // We create the window only at first call.
        // Note: If we create it in the constructor, the window gets created
        // in the wrong gui context and is never shown...
        createHelpWindow();
        mHelpWindow->show();
        break;

    case OIS::KC_F11:
        frameListener.toggleDebugInfo();
        break;

    case OIS::KC_GRAVE:
    case OIS::KC_F12:
        mModeManager->requestConsoleMode();
        frameListener.setTerminalActive(true);
        Console::getSingleton().setVisible(true);
        break;

    case OIS::KC_LEFT:
    case OIS::KC_A:
        inputManager->mDirectionKeyPressed = true;
        frameListener.moveCamera(CameraManager::Direction::moveLeft);
        break;

    case OIS::KC_RIGHT:
    case OIS::KC_D:
        inputManager->mDirectionKeyPressed = true;
        frameListener.moveCamera(CameraManager::Direction::moveRight);
        break;

    case OIS::KC_UP:
    case OIS::KC_W:
        inputManager->mDirectionKeyPressed = true;
        frameListener.moveCamera(CameraManager::Direction::moveForward);
        break;

    case OIS::KC_DOWN:
    case OIS::KC_S:
        inputManager->mDirectionKeyPressed = true;
        frameListener.moveCamera(CameraManager::Direction::moveBackward);
        break;

    case OIS::KC_Q:
        frameListener.moveCamera(CameraManager::Direction::rotateLeft);
        break;

    case OIS::KC_E:
        frameListener.moveCamera(CameraManager::Direction::rotateRight);
        break;

    case OIS::KC_HOME:
        frameListener.moveCamera(CameraManager::Direction::moveDown);
        break;

    case OIS::KC_END:
        frameListener.moveCamera(CameraManager::Direction::moveUp);
        break;

    case OIS::KC_PGUP:
        frameListener.moveCamera(CameraManager::Direction::rotateUp);
        break;

    case OIS::KC_PGDOWN:
        frameListener.moveCamera(CameraManager::Direction::rotateDown);
        break;

    case OIS::KC_T:
        if(isConnected()) // If we are in a game.
        {
            Seat* tempSeat = mGameMap->getLocalPlayer()->getSeat();
            frameListener.cameraFlyTo(tempSeat->getStartingPosition());
        }
        break;

    // Quit the game
    case OIS::KC_ESCAPE:
        popupExit(!mGameMap->getGamePaused());
        break;

    // Print a screenshot
    case OIS::KC_SYSRQ:
        ResourceManager::getSingleton().takeScreenshot(frameListener.getRenderWindow());
        break;

    case OIS::KC_RETURN:
        mCurrentInputMode = InputModeChat;
        ODFrameListener::getSingleton().notifyChatInputMode(true);
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

bool GameMode::keyPressedChat(const OIS::KeyEvent &arg)
{
    mKeysChatPressed.push_back(arg.key);
    if(arg.key == OIS::KC_RETURN || arg.key == OIS::KC_ESCAPE)
    {
        mCurrentInputMode = InputModeNormal;
        ODFrameListener::getSingleton().notifyChatInputMode(false, arg.key == OIS::KC_RETURN);
    }
    else if(arg.key == OIS::KC_BACK)
    {
        ODFrameListener::getSingleton().notifyChatCharDel();
    }
    else
    {
        ODFrameListener::getSingleton().notifyChatChar(getChatChar(arg));
    }
    return true;
}

bool GameMode::keyReleased(const OIS::KeyEvent &arg)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyUp((CEGUI::Key::Scan) arg.key);

    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    if (frameListener->isTerminalActive())
        return true;

    if(std::find(mKeysChatPressed.begin(), mKeysChatPressed.end(), arg.key) != mKeysChatPressed.end())
        return keyReleasedChat(arg);

    return keyReleasedNormal(arg);
}

bool GameMode::keyReleasedNormal(const OIS::KeyEvent &arg)
{
    ODFrameListener& frameListener = ODFrameListener::getSingleton();
    InputManager* inputManager = mModeManager->getInputManager();

    switch (arg.key)
    {
    case OIS::KC_LEFT:
    case OIS::KC_A:
        inputManager->mDirectionKeyPressed = false;
        frameListener.moveCamera(CameraManager::Direction::stopLeft);
        break;

    case OIS::KC_RIGHT:
    case OIS::KC_D:
        inputManager->mDirectionKeyPressed = false;
        frameListener.moveCamera(CameraManager::Direction::stopRight);
        break;

    case OIS::KC_UP:
    case OIS::KC_W:
        inputManager->mDirectionKeyPressed = false;
        frameListener.moveCamera(CameraManager::Direction::stopForward);
        break;

    case OIS::KC_DOWN:
    case OIS::KC_S:
        inputManager->mDirectionKeyPressed = false;
        frameListener.moveCamera(CameraManager::Direction::stopBackward);
        break;

    case OIS::KC_Q:
        frameListener.moveCamera(CameraManager::Direction::stopRotLeft);
        break;

    case OIS::KC_E:
        frameListener.moveCamera(CameraManager::Direction::stopRotRight);
        break;

    case OIS::KC_HOME:
        frameListener.cameraStopZoom();
        break;

    case OIS::KC_END:
        frameListener.cameraStopZoom();
        break;

    case OIS::KC_PGUP:
        frameListener.moveCamera(CameraManager::Direction::stopRotUp);
        break;

    case OIS::KC_PGDOWN:
        frameListener.moveCamera(CameraManager::Direction::stopRotDown);
        break;

    default:
        break;
    }

    return true;
}

bool GameMode::keyReleasedChat(const OIS::KeyEvent &arg)
{
    std::vector<OIS::KeyCode>::iterator it = std::find(mKeysChatPressed.begin(), mKeysChatPressed.end(), arg.key);
    if(it != mKeysChatPressed.end())
        mKeysChatPressed.erase(it);
    return true;
}

void GameMode::handleHotkeys(OIS::KeyCode keycode)
{
    ODFrameListener& frameListener = ODFrameListener::getSingleton();
    InputManager* inputManager = mModeManager->getInputManager();

    //keycode minus two because the codes are shifted by two against the actual number
    unsigned int keynumber = keycode - 2;

    if (getKeyboard()->isModifierDown(OIS::Keyboard::Shift))
    {
        inputManager->mHotkeyLocationIsValid[keynumber] = true;
        inputManager->mHotkeyLocation[keynumber] = frameListener.getCameraViewTarget();
    }
    else
    {
        if (inputManager->mHotkeyLocationIsValid[keynumber])
        {
            frameListener.cameraFlyTo(inputManager->mHotkeyLocation[keynumber]);
        }
    }
}

void GameMode::onFrameStarted(const Ogre::FrameEvent& evt)
{
    MiniMap* minimap = ODFrameListener::getSingleton().getMiniMap();
    minimap->draw();
    minimap->swap();
}

void GameMode::onFrameEnded(const Ogre::FrameEvent& evt)
{
}

void GameMode::popupExit(bool pause)
{
    if(pause)
    {
        Gui::getSingleton().getGuiSheet(Gui::inGameMenu)->getChild(Gui::EXIT_CONFIRMATION_POPUP)->show();
    }
    else
    {
        Gui::getSingleton().getGuiSheet(Gui::inGameMenu)->getChild(Gui::EXIT_CONFIRMATION_POPUP)->hide();
    }
    mGameMap->setGamePaused(pause);
}


void GameMode::exitMode()
{
    if(ODClient::getSingleton().isConnected())
        ODClient::getSingleton().disconnect();
    if(ODServer::getSingleton().isConnected())
        ODServer::getSingleton().stopServer();

    // Now that the server is stopped, we can clear the client game map
    ODFrameListener::getSingleton().getClientGameMap()->clearAll();
    ODFrameListener::getSingleton().getClientGameMap()->processDeletionQueues();
}

void GameMode::notifyGuiAction(GuiAction guiAction)
{
    switch(guiAction)
    {
            case GuiAction::ButtonPressedCreatureWorker:
            {
                if(ODClient::getSingleton().isConnected())
                {
                    ClientNotification *clientNotification = new ClientNotification(
                        ClientNotification::askPickupWorker);
                    ODClient::getSingleton().queueClientNotification(clientNotification);
                }
                break;
            }
            case GuiAction::ButtonPressedCreatureFighter:
            {
                if(ODClient::getSingleton().isConnected())
                {
                    ClientNotification *clientNotification = new ClientNotification(
                        ClientNotification::askPickupFighter);
                    ODClient::getSingleton().queueClientNotification(clientNotification);
                }
                break;
            }
            default:
                break;
    }
}

void GameMode::createHelpWindow()
{
    if (mHelpWindow != nullptr)
        return;

    CEGUI::WindowManager* wmgr = CEGUI::WindowManager::getSingletonPtr();
    CEGUI::Window* rootWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();

    mHelpWindow = wmgr->createWindow("OD/FrameWindow", std::string("GameHelpWindow"));
    mHelpWindow->setPosition(CEGUI::UVector2(CEGUI::UDim(0.2, 0), CEGUI::UDim(0.12, 0)));
    mHelpWindow->setSize(CEGUI::USize(CEGUI::UDim(0.6, 0), CEGUI::UDim(0.7, 0)));

    CEGUI::Window* textWindow = wmgr->createWindow("OD/StaticText", "TextDisplay");
    textWindow->setPosition(CEGUI::UVector2(CEGUI::UDim(0.05, 0), CEGUI::UDim(0.10, 0)));
    textWindow->setSize(CEGUI::USize(CEGUI::UDim(0.9, 0), CEGUI::UDim(0.9, 0)));
    textWindow->setProperty("FrameEnabled", "False");
    textWindow->setProperty("BackgroundEnabled", "False");
    textWindow->setProperty("VertFormatting", "TopAligned");
    textWindow->setProperty("HorzFormatting", "WordWrapLeftAligned");

    // Search for the autoclose button and make it work
    CEGUI::Window* childWindow = mHelpWindow->getChild("__auto_closebutton__");
    childWindow->subscribeEvent(CEGUI::PushButton::EventClicked,
                                CEGUI::Event::Subscriber(&GameMode::hideHelpWindow, this));

    // Set the window title
    childWindow = mHelpWindow->getChild("__auto_titlebar__");
    childWindow->setText("OpenDungeons Quick Help");

    mHelpWindow->addChild(textWindow);
    rootWindow->addChild(mHelpWindow);

    std::stringstream txt("");
    txt << "Welcome to the OpenDungeons quick help!" << std::endl << std::endl
        << "To move the camera: Use the arrow keys or W,A,S,D." << std::endl
        << "To rotate the camera, you can use either: A, or E." << std::endl
        << "Use the mouse wheel to go lower or higher, or Home, End." << std::endl
        << "And finally, you can use Page Up, Page Down, to look up or down." << std::endl << std::endl;
    txt << "You can left-click on the map's dirt walls to mark them. Your kobold workers will then "
        << "dig them for you. They will also claim tiles, turning them into stone with your color at their center." << std::endl
        << "Certain blocks are made of gold. You should look for them and make you workers dig those tiles in priority."
        << "Once you have enough gold, you can build room tiles that will permit to make your fighter creatures do many things "
        << "such as sleeping, eating, training, ... Certain rooms also attract new creatures types." << std::endl;
    txt << "Gold taken by your workers is put in your treasury rooms. If you haven't any, the first treasury room square tile is free of charge..."
        << std::endl << std::endl
        << "You can also left-click on one of your creatures to pick it up and right click somewhere else to put it back. "
        << "Very useful to help a creature in battle or force a worker to do a specific task..." << std::endl
        << "Note that you can place workers on any of your claimed tiles and unclaimed dirt tiles, "
        << "but you can place fighters only on claimed tiles and nothing at all on enemy claimed tiles." << std::endl;
    txt << "Your workers will also fortify walls, turning them into your color. Those cannot be broken by enemies until no more "
        << "claimed tiles around are of your color." << std::endl
        << "Last but not least, you can also grab gold left on the floor once your workers have claimed the corresponding tile, "
        << "and you can build rooms/drop creatures on allied claimed tiles (when in the same team). Don't forget you can also build traps!";
    txt << std::endl << std::endl << "Be evil, be cunning, your opponents will do the same, and have fun! ;)";
    textWindow->setText(txt.str());
}

bool GameMode::hideHelpWindow(const CEGUI::EventArgs& /*e*/)
{
    if (mHelpWindow != nullptr)
        mHelpWindow->hide();
    return true;
}
