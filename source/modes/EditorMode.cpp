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

#include "modes/GameEditorModeConsole.h"

#include "gamemap/GameMap.h"
#include "gamemap/MiniMap.h"
#include "gamemap/MapLoader.h"
#include "gamemap/Pathfinding.h"
#include "render/ODFrameListener.h"
#include "render/Gui.h"
#include "render/TextRenderer.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/MapLight.h"
#include "entities/Tile.h"
#include "game/ResearchManager.h"
#include "game/Seat.h"
#include "traps/TrapManager.h"
#include "game/Player.h"
#include "render/RenderManager.h"
#include "camera/CameraManager.h"
#include "rooms/RoomManager.h"
#include "rooms/RoomType.h"
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
    mCurrentCreatureIndex(0),
    mMouseX(0),
    mMouseY(0),
    mSettings(SettingsWindow(mRootWindow))
{
    // Set per default the input on the map
    mModeManager->getInputManager().mMouseDownOnCEGUIWindow = false;

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
        mRootWindow->getChild("EditorOptionsWindow/SettingsButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&EditorMode::showSettingsFromOptions, this)
        )
    );
    addEventConnection(
        mRootWindow->getChild("EditorOptionsWindow/QuitEditorButton")->subscribeEvent(
            CEGUI::Window::EventMouseClick,
            CEGUI::Event::Subscriber(&EditorMode::showQuitMenuFromOptions, this)
    ));

    // Connect editor specific buttons (Rooms, traps, spells, tiles, lights, ...)

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

    addEventConnection(
        mRootWindow->getChild(Gui::BUTTON_TEMPLE)->subscribeEvent(
          CEGUI::PushButton::EventClicked,
          CEGUI::Event::Subscriber(RoomSelector(RoomType::dungeonTemple, mPlayerSelection))
        )
    );

    addEventConnection(
        mRootWindow->getChild(Gui::BUTTON_PORTAL)->subscribeEvent(
          CEGUI::PushButton::EventClicked,
          CEGUI::Event::Subscriber(RoomSelector(RoomType::portal, mPlayerSelection))
        )
    );

    updateFlagColor();

    syncTabButtonTooltips(Gui::EDITOR);
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
    getModeManager().getInputManager().mSeatIdSelected = player->getSeat()->getId();

    refreshGuiResearch();

    mGameMap->setGamePaused(false);
}

bool EditorMode::mouseMoved(const OIS::MouseEvent &arg)
{
    AbstractApplicationMode::mouseMoved(arg);

    if (!isConnected())
        return true;

    InputManager& inputManager = mModeManager->getInputManager();
    inputManager.mCommandState = (inputManager.mLMouseDown ? InputCommandState::building : InputCommandState::infoOnly);

    // If we have a room/trap/spell selected, show it
    // TODO: This should be changed, or combined with an icon or something later.
    TextRenderer& textRenderer = TextRenderer::getSingleton();
    textRenderer.moveText(ODApplication::POINTER_INFO_STRING,
        static_cast<Ogre::Real>(arg.state.X.abs + 30), static_cast<Ogre::Real>(arg.state.Y.abs));

    // We notify current selection input
    checkInputCommand();

    handleMouseWheel(arg);

    // Since this is a tile selection query we loop over the result set
    // and look for the first object which is actually a tile.
    Ogre::Vector3 keeperHandPos;
    if(!ODFrameListener::getSingleton().findWorldPositionFromMouse(arg, keeperHandPos))
        return true;

    RenderManager::getSingleton().moveWorldCoords(keeperHandPos.x, keeperHandPos.y);

    int tileX = Helper::round(keeperHandPos.x);
    int tileY = Helper::round(keeperHandPos.y);
    Tile* tileClicked = mGameMap->getTile(tileX, tileY);
    if(tileClicked == nullptr)
        return true;

    std::vector<EntityBase*> entities;
    tileClicked->fillWithEntities(entities, SelectionEntityWanted::creatureAlive, mGameMap->getLocalPlayer());
    // We search the closest creature alive
    Creature* closestCreature = nullptr;
    double closestDist = 0;
    for(EntityBase* entity : entities)
    {
        if(entity->getObjectType() != GameEntityType::creature)
        {
            OD_LOG_ERR("entityName=" + entity->getName() + ", entityType=" + Helper::toString(static_cast<uint32_t>(entity->getObjectType())));
            continue;
        }

        const Ogre::Vector3& entityPos = entity->getPosition();
        double dist = Pathfinding::squaredDistance(entityPos.x, keeperHandPos.x, entityPos.y, keeperHandPos.y);
        if(closestCreature == nullptr)
        {
            closestDist = dist;
            closestCreature = static_cast<Creature*>(entity);
            continue;
        }

        if(dist >= closestDist)
            continue;

        closestDist = dist;
        closestCreature = static_cast<Creature*>(entity);
    }

    if(closestCreature != nullptr)
    {
        RenderManager::getSingleton().rrTemporaryDisplayCreaturesTextOverlay(closestCreature, 0.5f);
    }

    inputManager.mXPos = tileClicked->getX();
    inputManager.mYPos = tileClicked->getY();
    if (mMouseX != inputManager.mXPos || mMouseY != inputManager.mYPos)
    {
        mMouseX = inputManager.mXPos;
        mMouseY = inputManager.mYPos;
        updateCursorText();
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

    InputManager& inputManager = mModeManager->getInputManager();

    // If the mouse press is on a CEGUI window ignore it
    if (tempWindow != nullptr && tempWindow->getName().compare("EDITORGUI") != 0)
    {
        inputManager.mMouseDownOnCEGUIWindow = true;
        return true;
    }

    inputManager.mMouseDownOnCEGUIWindow = false;

    if(mGameMap->getLocalPlayer() == nullptr)
    {
        static bool log = true;
        if(log)
        {
            log = false;
            OD_LOG_ERR("LOCAL PLAYER DOES NOT EXIST!!");
        }
        return true;
    }

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

    Ogre::Vector3 keeperHandPos;
    if(!ODFrameListener::getSingleton().findWorldPositionFromMouse(arg, keeperHandPos))
        return true;

    RenderManager::getSingleton().moveWorldCoords(keeperHandPos.x, keeperHandPos.y);

    int tileX = Helper::round(keeperHandPos.x);
    int tileY = Helper::round(keeperHandPos.y);
    Tile* tileClicked = mGameMap->getTile(tileX, tileY);
    if(tileClicked == nullptr)
        return true;

    if (id == OIS::MB_Middle)
    {
        // See if the mouse is over any entity that might display a stats window
        std::vector<EntityBase*> entities;
        tileClicked->fillWithEntities(entities, SelectionEntityWanted::any, mGameMap->getLocalPlayer());
        // We search the closest creature alive
        EntityBase* closestEntity = nullptr;
        double closestDist = 0;
        for(EntityBase* entity : entities)
        {
            if(!entity->canDisplayStatsWindow(mGameMap->getLocalPlayer()->getSeat()))
                continue;

            const Ogre::Vector3& entityPos = entity->getPosition();
            double dist = Pathfinding::squaredDistance(entityPos.x, keeperHandPos.x, entityPos.y, keeperHandPos.y);
            if(closestEntity == nullptr)
            {
                closestDist = dist;
                closestEntity = entity;
                continue;
            }

            if(dist >= closestDist)
                continue;

            closestDist = dist;
            closestEntity = entity;
        }

        if(closestEntity == nullptr)
            return true;

        closestEntity->createStatsWindow();

        return true;
    }

    // Right mouse button down
    if (id == OIS::MB_Right)
    {
        inputManager.mRMouseDown = true;
        inputManager.mRStartDragX = inputManager.mXPos;
        inputManager.mRStartDragY = inputManager.mYPos;

        // Stop creating rooms, traps, etc.
        unselectAllTiles();
        mCurrentTileVisual = TileVisual::nullTileVisual;
        TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "");
        // If we have a currently selected action, we cancel it and don't try to slap or
        // drop what we have in hand
        if(mPlayerSelection.getCurrentAction() != SelectedAction::none)
        {
            mPlayerSelection.setCurrentAction(SelectedAction::none);
            return true;
        }

        // If we right clicked with the mouse over a valid map tile, try to drop a creature onto the map.
        Tile* curTile = mGameMap->getTile(inputManager.mXPos, inputManager.mYPos);

        if (curTile == nullptr)
            return true;


        if(mGameMap->getLocalPlayer()->numObjectsInHand() > 0)
        {
            // If we right clicked with the mouse over a valid map tile, try to drop what we have in hand on the map.
            Tile* curTile = mGameMap->getTile(inputManager.mXPos, inputManager.mYPos);

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
            std::vector<EntityBase*> entities;
            tileClicked->fillWithEntities(entities, SelectionEntityWanted::any, mGameMap->getLocalPlayer());
            // We search the closest creature alive
            EntityBase* closestEntity = nullptr;
            double closestDist = 0;
            for(EntityBase* entity : entities)
            {
                if(!entity->canSlap(mGameMap->getLocalPlayer()->getSeat()))
                    continue;

                const Ogre::Vector3& entityPos = entity->getPosition();
                double dist = Pathfinding::squaredDistance(entityPos.x, keeperHandPos.x, entityPos.y, keeperHandPos.y);
                if(closestEntity == nullptr)
                {
                    closestDist = dist;
                    closestEntity = entity;
                    continue;
                }

                if(dist >= closestDist)
                    continue;

                closestDist = dist;
                closestEntity = entity;
            }

            if(closestEntity != nullptr)
            {
                ODClient::getSingleton().queueClientNotification(ClientNotificationType::askSlapEntity,
                     closestEntity->getObjectType(),
                     closestEntity->getName());
                return true;
            }
        }
    }

    if (id != OIS::MB_Left)
        return true;

    // Left mouse button down
    inputManager.mLMouseDown = true;
    inputManager.mLStartDragX = inputManager.mXPos;
    inputManager.mLStartDragY = inputManager.mYPos;

    // Check whether the player is already placing rooms or traps.
    if (mPlayerSelection.getCurrentAction() != SelectedAction::none)
    {
        // Skip picking up creatures when placing rooms or traps
        // as creatures often get in the way.
        return true;
    }

    // See if the mouse is over any pickup-able entity
    std::vector<EntityBase*> entities;
    tileClicked->fillWithEntities(entities, SelectionEntityWanted::any, mGameMap->getLocalPlayer());
    // We search the closest creature alive
    EntityBase* closestEntity = nullptr;
    double closestDist = 0;
    for(EntityBase* entity : entities)
    {
        if(!entity->canSlap(mGameMap->getLocalPlayer()->getSeat()))
            continue;

        const Ogre::Vector3& entityPos = entity->getPosition();
        double dist = Pathfinding::squaredDistance(entityPos.x, keeperHandPos.x, entityPos.y, keeperHandPos.y);
        if(closestEntity == nullptr)
        {
            closestDist = dist;
            closestEntity = entity;
            continue;
        }

        if(dist >= closestDist)
            continue;

        closestDist = dist;
        closestEntity = entity;
    }

    if(closestEntity != nullptr)
    {
        ODClient::getSingleton().queueClientNotification(ClientNotificationType::askEntityPickUp,
            closestEntity->getObjectType(),
            closestEntity->getName());
        return true;
    }

    return true;
}

bool EditorMode::mouseReleased(const OIS::MouseEvent& arg, OIS::MouseButtonID id)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(Gui::convertButton(id));

    InputManager& inputManager = mModeManager->getInputManager();
    inputManager.mCommandState = InputCommandState::validated;
    // If the mouse press was on a CEGUI window ignore it
    if (inputManager.mMouseDownOnCEGUIWindow)
        return true;

    // Right mouse button up
    if (id == OIS::MB_Right)
    {
        inputManager.mRMouseDown = false;
        return true;
    }

    if (id != OIS::MB_Left)
        return true;

    // Left mouse button up
    inputManager.mLMouseDown = false;

    // We notify current selection input
    checkInputCommand();

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
    textSS << "Seat id (Y): " << getModeManager().getInputManager().mSeatIdSelected;
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
    // Inject key to the gui currently displayed
    CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyDown(static_cast<CEGUI::Key::Scan>(arg.key));
    CEGUI::System::getSingleton().getDefaultGUIContext().injectChar(arg.text);

    if (mCurrentInputMode == InputModeChat)
        return true;

    if (mCurrentInputMode == InputModeConsole)
        return getConsole()->keyPressed(arg);

    ODFrameListener& frameListener = ODFrameListener::getSingleton();

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
        enterConsole();
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

    //Toggle selected seat ID
    case OIS::KC_Y:
        getModeManager().getInputManager().mSeatIdSelected = mGameMap->nextSeatId(getModeManager().getInputManager().mSeatIdSelected);
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

    if (mCurrentInputMode == InputModeChat || mCurrentInputMode == InputModeConsole)
        return true;

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
    InputManager& inputManager = mModeManager->getInputManager();

    if (getKeyboard()->isModifierDown(OIS::Keyboard::Shift))
    {
        inputManager.mHotkeyLocationIsValid[keynumber] = true;
        inputManager.mHotkeyLocation[keynumber] = frameListener.getCameraViewTarget();
    }
    else
    {
        if (inputManager.mHotkeyLocationIsValid[keynumber])
            frameListener.cameraFlyTo(inputManager.mHotkeyLocation[keynumber]);
    }
}

//! Rendering methods
void EditorMode::onFrameStarted(const Ogre::FrameEvent& evt)
{
    GameEditorModeBase::onFrameStarted(evt);
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
                    clientNotification->mPacket << getModeManager().getInputManager().mSeatIdSelected;
                    ODClient::getSingleton().queueClientNotification(clientNotification);
                }
                break;
            }
            case GuiAction::ButtonPressedCreatureFighter:
            {
                if(ODClient::getSingleton().isConnected())
                {
                    const CreatureDefinition* def = mGameMap->getClassDescription(mCurrentCreatureIndex);
                    if(def == nullptr)
                    {
                        OD_LOG_ERR("unexpected null CreatureDefinition mCurrentCreatureIndex=" + Helper::toString(mCurrentCreatureIndex));
                        break;
                    }
                    ClientNotification *clientNotification = new ClientNotification(
                        ClientNotificationType::editorCreateFighter);
                    clientNotification->mPacket << getModeManager().getInputManager().mSeatIdSelected;
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

bool EditorMode::showSettingsFromOptions(const CEGUI::EventArgs& /*e*/)
{
    mRootWindow->getChild("EditorOptionsWindow")->hide();
    mSettings.show();
    return true;
}

bool EditorMode::showQuitMenuFromOptions(const CEGUI::EventArgs& /*arg*/)
{
    mRootWindow->getChild("ConfirmExit")->show();
    mRootWindow->getChild("EditorOptionsWindow")->hide();
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
    mModeManager->requestMode(AbstractModeManager::MENU_MAIN);
    return true;
}

void EditorMode::refreshGuiResearch()
{
    // We show/hide the icons depending on available researches
    CEGUI::Window* guiSheet = mRootWindow;

    ResearchManager::listAllResearches([&](const std::string& researchButtonName, const std::string& castButtonName,
        const std::string& researchProgressBarName, ResearchType resType)
    {
        guiSheet->getChild(castButtonName)->show();
    });


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
    std::string colorStr = Helper::getImageColoursStringFromColourValue(
        mGameMap->getSeatById(getModeManager().getInputManager().mSeatIdSelected)->getColorValue());
    mRootWindow->getChild("HorizontalPipe/SeatIdDisplay/Icon")->setProperty("ImageColours", colorStr);
}

void EditorMode::selectSquaredTiles(int tileX1, int tileY1, int tileX2, int tileY2)
{
    // Loop over the tiles in the rectangular selection region and set their setSelected flag accordingly.
    std::vector<Tile*> affectedTiles = mGameMap->rectangularRegion(tileX1,
        tileY1, tileX2, tileY2);

    selectTiles(affectedTiles);
}

void EditorMode::selectTiles(const std::vector<Tile*> tiles)
{
    unselectAllTiles();

    Player* player = mGameMap->getLocalPlayer();
    for(Tile* tile : tiles)
    {
        tile->setSelected(true, player);
    }
}

void EditorMode::unselectAllTiles()
{
    Player* player = mGameMap->getLocalPlayer();
    // Compute selected tiles
    for (int jj = 0; jj < mGameMap->getMapSizeY(); ++jj)
    {
        for (int ii = 0; ii < mGameMap->getMapSizeX(); ++ii)
        {
            mGameMap->getTile(ii, jj)->setSelected(false, player);
        }
    }
}

void EditorMode::displayText(const Ogre::ColourValue& txtColour, const std::string& txt)
{
    TextRenderer& textRenderer = TextRenderer::getSingleton();
    textRenderer.setColor(ODApplication::POINTER_INFO_STRING, txtColour);
    textRenderer.setText(ODApplication::POINTER_INFO_STRING, txt);
}

void EditorMode::checkInputCommand()
{
    // In the editor mode, by default, we do nothing if mouse dragged while no action selected
    const InputManager& inputManager = mModeManager->getInputManager();

    switch(mPlayerSelection.getCurrentAction())
    {
        case SelectedAction::none:
            handlePlayerActionNone();
            return;
        case SelectedAction::changeTile:
            handlePlayerActionChangeTile();
            return;
        case SelectedAction::buildRoom:
            RoomManager::checkBuildRoomEditor(mGameMap, mPlayerSelection.getNewRoomType(), inputManager, *this);
            return;
        case SelectedAction::destroyRoom:
            RoomManager::checkSellRoomTilesEditor(mGameMap, inputManager, *this);
            return;
        case SelectedAction::buildTrap:
            TrapManager::checkBuildTrapEditor(mGameMap, mPlayerSelection.getNewTrapType(), inputManager, *this);
            return;
        case SelectedAction::destroyTrap:
            TrapManager::checkSellTrapTilesEditor(mGameMap, inputManager, *this);
            return;
        case SelectedAction::castSpell:
            // TODO: create research entity with corresponding spell
            return;
        default:
            return;
    }
}

void EditorMode::handlePlayerActionChangeTile()
{
    const InputManager& inputManager = mModeManager->getInputManager();
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos,
            inputManager.mYPos);
        displayText(Ogre::ColourValue::White, Tile::tileVisualToString(mCurrentTileVisual));
        return;
    }

    if(inputManager.mCommandState == InputCommandState::building)
    {
        selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mLStartDragX,
            inputManager.mLStartDragY);
        displayText(Ogre::ColourValue::White, Tile::tileVisualToString(mCurrentTileVisual));
        return;
    }

    unselectAllTiles();
    displayText(Ogre::ColourValue::White, "");

    TileType tileType = TileType::nullTileType;
    double fullness = mCurrentFullness;
    int seatId = -1;
    switch(mCurrentTileVisual)
    {
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
            seatId = inputManager.mSeatIdSelected;
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
            return;
    }

    ClientNotification *clientNotification = new ClientNotification(
        ClientNotificationType::editorAskChangeTiles);
    clientNotification->mPacket << inputManager.mXPos << inputManager.mYPos;
    clientNotification->mPacket << inputManager.mLStartDragX << inputManager.mLStartDragY;
    clientNotification->mPacket << tileType;
    clientNotification->mPacket << fullness;
    clientNotification->mPacket << seatId;
    ODClient::getSingleton().queueClientNotification(clientNotification);
}

void EditorMode::handlePlayerActionNone()
{
    const InputManager& inputManager = mModeManager->getInputManager();
    // We only display the selection cursor on the hovered tile
    if(inputManager.mCommandState == InputCommandState::validated)
    {
        unselectAllTiles();
        return;
    }

    selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos,
        inputManager.mYPos);
}
