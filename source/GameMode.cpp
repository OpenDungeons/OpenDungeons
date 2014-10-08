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

#include "GameMode.h"

#include "ODClient.h"
#include "Gui.h"
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
#include "ODServer.h"
#include "ODApplication.h"
#include "RoomObject.h"
#include "MiniMap.h"
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
    mCurrentInputMode(InputModeNormal)
{
    // Set per default the input on the map
    mModeManager->getInputManager()->mMouseDownOnCEGUIWindow = false;

    // Keep track of the mouse light object
    Ogre::SceneManager* sceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
    mMouseLight = sceneMgr->getLight("MouseLight");
}

GameMode::~GameMode()
{
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

    // Make the mouse light follow the mouse
    mMouseLight->setPosition((Ogre::Real)mMouseX, (Ogre::Real)mMouseY, 2.0);

    // Make the square selector follow the mouse
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::showSquareSelector;
    request->p = static_cast<void*>(&mMouseX);
    request->p2 = static_cast<void*>(&mMouseY);
    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request); // NOTE: will delete the request member for us.
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

                for(std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end(); ++it)
                {
                    Tile* tile = *it;
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

                for(std::vector<Tile*>::iterator it = tiles.begin(); it != tiles.end(); ++it)
                {
                    Tile* tile = *it;
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
        mGameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::none);
        mGameMap->getLocalPlayer()->setNewRoomType(Room::nullRoomType);
        mGameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
        TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "");

        // If we right clicked with the mouse over a valid map tile, try to drop a creature onto the map.
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

        if (resultName.find(Creature::CREATURE_PREFIX) != std::string::npos)
        {
            // Pick the creature up and put it in our hand
            if(inputManager->mExpectCreatureClick)
            {
                // TODO : switch to FPP mode
#if 0 // FPP mode do not exist yet but we keep track of what was done.
                mModeManager->requestFppMode();
                const string& tmp_name =  (itr->movable->getName());
                std::cerr << tmp_name.substr(9, tmp_name.size()) << std::endl;
                cm->setFPPCamera(mGameMap->getCreature(tmp_name.substr(9, tmp_name.size())));
                cm->setActiveCameraNode("FPP");
                cm->setActiveCamera("FPP");

#endif // 0
                inputManager->mExpectCreatureClick = false;
            }
            else
            {
                // The creature name is after the creature prefix
                std::string creatureName = resultName.substr(Creature::CREATURE_PREFIX.length());
                Creature* currentCreature = mGameMap->getCreature(creatureName);
                if (currentCreature == NULL)
                    continue;

                // Checks on client side that the creature can be picked up
                if ((currentCreature->tryPickup(player->getSeat(), false)) && ODClient::getSingleton().isConnected())
                {
                    // Send a message to the server telling it we want to pick up this creature
                    ClientNotification *clientNotification = new ClientNotification(
                        ClientNotification::askEntityPickUp);
                    const std::string& name = currentCreature->getName();
                    clientNotification->mPacket << GameEntity::ObjectType::creature;
                    clientNotification->mPacket << name;
                    ODClient::getSingleton().queueClientNotification(clientNotification);
                    return true;
                }
            }
            continue;
        }

        if (resultName.find(RoomObject::ROOMOBJECT_OGRE_PREFIX) != std::string::npos)
        {
            // The creature name is after the creature prefix
            std::string name = resultName.substr(RoomObject::ROOMOBJECT_OGRE_PREFIX.length());
            RoomObject* obj = mGameMap->getRoomObject(name);
            if ((obj == NULL) || (!obj->tryPickup(player->getSeat(), false)))
                continue;

            // Checks on client side that the creature can be picked up
            if (ODClient::getSingleton().isConnected())
            {
                // Send a message to the server telling it we want to pick up this creature
                ClientNotification *clientNotification = new ClientNotification(
                    ClientNotification::askEntityPickUp);
                const std::string& name = obj->getName();
                clientNotification->mPacket << GameEntity::ObjectType::roomobject;
                clientNotification->mPacket << name;
                ODClient::getSingleton().queueClientNotification(clientNotification);
                return true;
            }
            continue;
        }
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
            frameListener.cameraFlyTo(Ogre::Vector3((Ogre::Real)tempSeat->mStartingX,
                                                   (Ogre::Real)tempSeat->mStartingY,
                                                   (Ogre::Real)0.0));
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
    // We process RenderRequests in case there is graphical things pending
    RenderManager::getSingleton().processRenderRequests();
    ODFrameListener::getSingleton().getClientGameMap()->clearAll();
    // We process again RenderRequests to destroy/delete what clearAll has put in the queue
    RenderManager::getSingleton().processRenderRequests();
}
