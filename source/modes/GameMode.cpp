/*!
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#include "camera/CameraManager.h"
#include "entities/Creature.h"
#include "entities/GameEntityType.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Skill.h"
#include "game/SkillManager.h"
#include "game/Seat.h"
#include "game/SkillType.h"
#include "gamemap/GameMap.h"
#include "gamemap/Pathfinding.h"
#include "modes/GameEditorModeConsole.h"
#include "modes/InputBridge.h"
#include "network/ChatEventMessage.h"
#include "network/ODClient.h"
#include "network/ODServer.h"
#include "render/Gui.h"
#include "render/ODFrameListener.h"
#include "render/RenderManager.h"
#include "render/TextRenderer.h"
#include "rooms/RoomManager.h"
#include "rooms/RoomType.h"
#include "sound/MusicPlayer.h"
#include "sound/SoundEffectsManager.h"
#include "spells/SpellManager.h"
#include "spells/SpellType.h"
#include "traps/Trap.h"
#include "traps/TrapManager.h"
#include "traps/TrapType.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/ResourceManager.h"
#include "ODApplication.h"

#include <CEGUI/CEGUI.h>
#include <CEGUI/WindowManager.h>
#include <CEGUI/widgets/PushButton.h>
#include <CEGUI/widgets/ToggleButton.h>
#include <CEGUI/widgets/ProgressBar.h>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>

#include <algorithm>
#include <vector>
#include <string>

const std::string TEXT_SEAT_ID_PREFIX = "TextSeat";
const std::string TEXT_SEAT_PLAYER_NICKNAME_PREFIX = "TextSeatPlayerNick";
const std::string TEXT_SEAT_TEAM_ID_PREFIX = "TextSeatTeam";

GameMode::GameMode(ModeManager *modeManager):
    GameEditorModeBase(modeManager, ModeManager::GAME, modeManager->getGui().getGuiSheet(Gui::guiSheet::inGameMenu)),
    mDigSetBool(false),
    mIndexEvent(0),
    mSettings(SettingsWindow(mRootWindow)),
    mIsSkillWindowOpen(false),
    mCurrentSkillType(SkillType::nullSkillType),
    mCurrentSkillProgress(0.0),
    mPreviousMousePosition(MouseMoveEvent{0, 0}),
    directionKeyPressed(false),
    config(ConfigManager::getSingleton())
{
    // Set per default the input on the map
    mModeManager->getInputManager().mMouseDownOnCEGUIWindow = false;

    ODFrameListener::getSingleton().getCameraManager()->setDefaultView();

    CEGUI::Window* guiSheet = mRootWindow;

    //Help window
    addEventConnection(
        guiSheet->getChild("HelpButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::toggleHelpWindow, this)
        )
    );

    //Objectives window
    addEventConnection(
        guiSheet->getChild("ObjectivesButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::toggleObjectivesWindow, this)
        )
    );
    addEventConnection(
        guiSheet->getChild("ObjectivesWindow")->subscribeEvent(
            CEGUI::FrameWindow::EventCloseClicked,
            CEGUI::Event::Subscriber(&GameMode::hideObjectivesWindow, this)
        )
    );

    //Player settings window
    addEventConnection(
        guiSheet->getChild("PlayerSettingsButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::togglePlayerSettingsWindow, this)
        )
    );
    addEventConnection(
        guiSheet->getChild("PlayerSettingsWindow")->subscribeEvent(
            CEGUI::FrameWindow::EventCloseClicked,
            CEGUI::Event::Subscriber(&GameMode::cancelPlayerSettings, this)
        )
    );
    addEventConnection(
        guiSheet->getChild("PlayerSettingsWindow/CancelButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::cancelPlayerSettings, this)
        )
    );
    addEventConnection(
        guiSheet->getChild("PlayerSettingsWindow/ApplyButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::applyPlayerSettings, this)
        )
    );

    // The skill tree window
    addEventConnection(
        guiSheet->getChild("SkillButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::toggleSkillWindow, this)
        )
    );
    addEventConnection(
        guiSheet->getChild("SkillTreeWindow")->subscribeEvent(
            CEGUI::FrameWindow::EventCloseClicked,
            CEGUI::Event::Subscriber(&GameMode::hideSkillWindow, this)
        )
    );
    addEventConnection(
        guiSheet->getChild("SkillTreeWindow/AutoFill")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::autoFillSkillWindow, this)
        )
    );
    addEventConnection(
        guiSheet->getChild("SkillTreeWindow/UnselectAll")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::unselectAllSkillWindow, this)
        )
    );
    addEventConnection(
        guiSheet->getChild("SkillTreeWindow/CancelButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::hideSkillWindow, this)
        )
    );
    addEventConnection(
        guiSheet->getChild("SkillTreeWindow/ApplyButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::applySkillWindow, this)
        )
    );

    // The Game Option menu events
    addEventConnection(
        guiSheet->getChild("OptionsButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::toggleOptionsWindow, this)
        )
    );
    addEventConnection(
        guiSheet->getChild("GameOptionsWindow")->subscribeEvent(
            CEGUI::FrameWindow::EventCloseClicked,
            CEGUI::Event::Subscriber(&GameMode::hideOptionsWindow, this)
        )
    );
    addEventConnection(
        guiSheet->getChild("GameOptionsWindow/ObjectivesButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::showObjectivesFromOptions, this)
        )
    );
    addEventConnection(
        guiSheet->getChild("GameOptionsWindow/SkillButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::showSkillFromOptions, this)
        )
    );
    CEGUI::Window* saveGameButtonWindow = guiSheet->getChild("GameOptionsWindow/SaveGameButton");
    addEventConnection(
        saveGameButtonWindow->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::saveGame, this)
        )
    );
    saveGameButtonWindow->setEnabled(ODServer::getSingleton().isConnected());
    addEventConnection(
        guiSheet->getChild("GameOptionsWindow/SettingsButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::showSettingsFromOptions, this)
        )
    );
    addEventConnection(
        guiSheet->getChild("GameOptionsWindow/QuitGameButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::showQuitMenuFromOptions, this)
        )
    );

    //Exit confirmation box
    addEventConnection(
        guiSheet->getChild(Gui::EXIT_CONFIRMATION_POPUP_YES_BUTTON)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::onClickYesQuitMenu, this)
        )
    );

    //Exit confirmation box
    auto cancelExitWindow =
          [this](const CEGUI::EventArgs&)
          {
                  popupExit(false);
                  return true;
          };
    addEventConnection(
        guiSheet->getChild(Gui::EXIT_CONFIRMATION_POPUP_NO_BUTTON)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(cancelExitWindow)
        )
    );
    addEventConnection(
        guiSheet->getChild("ConfirmExit")->subscribeEvent(
            CEGUI::FrameWindow::EventCloseClicked,
            CEGUI::Event::Subscriber(cancelExitWindow)
        )
    );

    // Help window
    addEventConnection(
        guiSheet->getChild("GameHelpWindow")->subscribeEvent(
            CEGUI::FrameWindow::EventCloseClicked,
            CEGUI::Event::Subscriber(&GameMode::hideHelpWindow, this)
        )
    );

    // Set the help window text
    setHelpWindowText();

    SkillManager::connectSkills(this, mRootWindow);

    syncTabButtonTooltips(Gui::MAIN_TABCONTROL);
}

GameMode::~GameMode()
{
    CEGUI::ToggleButton* checkBox =
        dynamic_cast<CEGUI::ToggleButton*>(
            mRootWindow->getChild(
                Gui::EXIT_CONFIRMATION_POPUP)->getChild("SaveReplayCheckbox"));

    mKeepReplayAtDisconnect = checkBox->isSelected();

    CEGUI::Window* settingsWin = mRootWindow->getChild("PlayerSettingsWindow/Seats/SeatsSP");
    CEGUI::WindowManager& winMgr = CEGUI::WindowManager::getSingleton();
    for(int seatId : mSeatIds)
    {
        CEGUI::Window* tmpWin;
        std::string name;
        name = TEXT_SEAT_ID_PREFIX + Helper::toString(seatId);
        tmpWin = settingsWin->getChild(name);
        settingsWin->removeChild(tmpWin);
        winMgr.destroyWindow(tmpWin);

        name = TEXT_SEAT_PLAYER_NICKNAME_PREFIX + Helper::toString(seatId);
        tmpWin = settingsWin->getChild(name);
        settingsWin->removeChild(tmpWin);
        winMgr.destroyWindow(tmpWin);

        name = TEXT_SEAT_TEAM_ID_PREFIX + Helper::toString(seatId);
        tmpWin = settingsWin->getChild(name);
        settingsWin->removeChild(tmpWin);
        winMgr.destroyWindow(tmpWin);
    }
}

void GameMode::activate()
{
    // Loads the corresponding Gui sheet.
    Gui& gui = getModeManager().getGui();
    gui.loadGuiSheet(Gui::inGameMenu);

    // We free the menu scene as it is not required anymore
    ODFrameListener::getSingleton().freeMainMenuScene();

    buildPlayerSettingsWindow();

    // Hides the exit pop-up and certain buttons only used by the editor.
    CEGUI::Window* guiSheet = mRootWindow;
    guiSheet->getChild(Gui::EXIT_CONFIRMATION_POPUP)->hide();
    guiSheet->getChild("ObjectivesWindow")->hide();
    guiSheet->getChild("PlayerSettingsWindow")->hide();
    guiSheet->getChild("SkillTreeWindow")->hide();
    guiSheet->getChild("SettingsWindow")->hide();
    guiSheet->getChild("GameOptionsWindow")->hide();
    guiSheet->getChild("GameChatWindow/GameChatEditBox")->hide();
    guiSheet->getChild("GameHelpWindow")->hide();

    giveFocus();

    // Play the game music.
    MusicPlayer::getSingleton().play(mGameMap->getLevelMusicFile()); // in game music

    std::string colorStr = Helper::getImageColoursStringFromColourValue(mGameMap->getLocalPlayer()->getSeat()->getColorValue());
    guiSheet->getChild("HorizontalPipe")->setProperty("ImageColours", colorStr);

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

    // Update available options
    refreshGuiSkill(true);

    syncPlayerSettings();
}

bool GameMode::mouseMoved(const OIS::MouseEvent &arg)
{
    AbstractApplicationMode::mouseMoved(arg);

    auto mouseEvent = toSFMLMouseMove(arg);
    auto mouseDelta = MouseMoveEvent{mPreviousMousePosition.x - mouseEvent.x, mPreviousMousePosition.y - mouseEvent.y};
    ODFrameListener::getSingleton().printDebugInfoTail.str(std::string());
    ODFrameListener::getSingleton().printDebugInfoTail << "mouseDelta x:" << mouseDelta.x << " y:" << mouseDelta.y << " " << std::endl;
    mPreviousMousePosition = mouseEvent;

    if (!isConnected())
        return true;

    InputManager& inputManager = mModeManager->getInputManager();
    inputManager.mCommandState = (inputManager.mLMouseDown ? InputCommandState::building : InputCommandState::infoOnly);

    // TODO: Here we should check whether the terminal is active...
    if(inputManager.mMMouseDown)
    {
        ODFrameListener::getSingleton().moveCamera(CameraManager::randomRotateX,mouseDelta.x);
        ODFrameListener::getSingleton().moveCamera(CameraManager::randomRotateY,mouseDelta.y);
    }

    if (!directionKeyPressed && config.getInputValue(Config::AUTOSCROLL, "No", false) == "Yes")
    {
        if (arg.state.X.abs == 0)
            ODFrameListener::getSingleton().moveCamera(CameraManager::moveLeft);
        else
            ODFrameListener::getSingleton().moveCamera(CameraManager::stopLeft);

        if (arg.state.X.abs >= 0.98 * arg.state.width)
            ODFrameListener::getSingleton().moveCamera(CameraManager::moveRight);
        else
            ODFrameListener::getSingleton().moveCamera(CameraManager::stopRight);

        if (arg.state.Y.abs == 0)
            ODFrameListener::getSingleton().moveCamera(CameraManager::moveForward);
        else
            ODFrameListener::getSingleton().moveCamera(CameraManager::stopForward);

        if (arg.state.Y.abs >= 0.98 * arg.state.height)
            ODFrameListener::getSingleton().moveCamera(CameraManager::moveBackward);
        else
            ODFrameListener::getSingleton().moveCamera(CameraManager::stopBackward);            
    }
 
    // If we have a room/trap/spell selected, show it
    // TODO: This should be changed, or combined with an icon or something later.
    TextRenderer& textRenderer = TextRenderer::getSingleton();
    textRenderer.moveText(ODApplication::POINTER_INFO_STRING,
        static_cast<Ogre::Real>(mouseEvent.x + 30), static_cast<Ogre::Real>(mouseEvent.y));

    // We notify current selection input
    checkInputCommand();

    handleMouseWheel(toSFMLMouseWheel(arg));

    // Since this is a tile selection query we loop over the result set
    // and look for the first object which is actually a tile.
    ODFrameListener::getSingleton().findWorldPositionFromMouse(arg, inputManager.mKeeperHandPos, inputManager.mKeeperHandGroundPos);
    RenderManager::getSingleton().moveWorldCoords(inputManager.mKeeperHandPos.x, inputManager.mKeeperHandPos.y);

    int tileX = Helper::round(inputManager.mKeeperHandPos.x);
    int tileY = Helper::round(inputManager.mKeeperHandPos.y);
    Tile* tileClicked = mGameMap->getTile(tileX, tileY);
    if(tileClicked == nullptr)
        return true;

    std::vector<GameEntity*> entities;
    tileClicked->fillWithEntities(entities, SelectionEntityWanted::creatureAliveOrDead, mGameMap->getLocalPlayer());
    // We search the closest creature alive
    Creature* closestCreature = nullptr;
    double closestDist = 0;
    for(GameEntity* entity : entities)
    {
        if(entity->getObjectType() != GameEntityType::creature)
        {
            OD_LOG_ERR("entityName=" + entity->getName() + ", entityType=" + Helper::toString(static_cast<uint32_t>(entity->getObjectType())));
            continue;
        }

        const Ogre::Vector3& entityPos = entity->getPosition();
        double dist = Pathfinding::squaredDistance(entityPos.x, inputManager.mKeeperHandPos.x, entityPos.y, inputManager.mKeeperHandPos.y);
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
        RenderManager::getSingleton().rrTemporaryDisplayCreaturesTextOverlay(closestCreature, 0.5f);

    inputManager.mXPos = tileClicked->getX();
    inputManager.mYPos = tileClicked->getY();
    if (!inputManager.mLMouseDown)
    {
        inputManager.mLStartDragX = inputManager.mXPos;
        inputManager.mLStartDragY = inputManager.mYPos;
    }
    return true;
}

void GameMode::handleMouseWheel(const MouseWheelEvent &arg)
{
    if(isMouseWheelOnCEGUIWindow())
        return;

    ODFrameListener& frameListener = ODFrameListener::getSingleton();

    if (arg.delta > 0)
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
    else if (arg.delta < 0)
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

bool GameMode::isMouseDownOnCEGUIWindow()
{
    CEGUI::Window* currentWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getWindowContainingMouse();

    if (currentWindow == nullptr)
        return false;

    CEGUI::String winName = currentWindow->getName();

    // If the mouse press is on a CEGUI window, ignore it, except for the chat and event queues windows.
    if (winName == "Root" || winName == "GameChatWindow" || winName == "GameChatText" || winName == "GameEventText")
        return false;

    return true;
}

bool GameMode::isMouseWheelOnCEGUIWindow()
{
    CEGUI::Window* currentWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getWindowContainingMouse();

    if (currentWindow == nullptr)
        return false;

    CEGUI::String winName = currentWindow->getName();

    // For the wheel, we do not ignore chat and events windows
    if (winName == "Root")
        return false;

    return true;
}

bool GameMode::mousePressed(const OIS::MouseEvent& arg, OIS::MouseButtonID id)
{
    InputManager& inputManager = mModeManager->getInputManager();

    CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
        Gui::convertButton(id));

    if (!isConnected())
        return true;

    inputManager.mMouseDownOnCEGUIWindow = isMouseDownOnCEGUIWindow();
    if (inputManager.mMouseDownOnCEGUIWindow)
        return true;

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

    if(mGameMap->getGamePaused())
        return true;

    if(!ODFrameListener::getSingleton().findWorldPositionFromMouse(arg, inputManager.mKeeperHandPos,  inputManager.mKeeperHandPos ))
        return true;

    RenderManager::getSingleton().moveWorldCoords(inputManager.mKeeperHandPos.x, inputManager.mKeeperHandPos.y);

    // The player should be able to move the mouse even if not clicking on a tile. Because of that, we set
    // mMMouseDown before checking which tile is clicked
    if (id == OIS::MB_Middle)
        inputManager.mMMouseDown = true;

    int tileX = Helper::round(inputManager.mKeeperHandPos.x);
    int tileY = Helper::round(inputManager.mKeeperHandPos.y);
    Tile* tileClicked = mGameMap->getTile(tileX, tileY);
    if(tileClicked == nullptr)
        return true;

    if (id == OIS::MB_Middle)
    {
        // See if the mouse is over any entity that might display a stats window
        std::vector<GameEntity*> entities;
        tileClicked->fillWithEntities(entities, SelectionEntityWanted::any, mGameMap->getLocalPlayer());
        // We search the closest creature alive
        GameEntity* closestEntity = nullptr;
        double closestDist = 0;
        for(GameEntity* entity : entities)
        {
            if(!entity->canDisplayStatsWindow(mGameMap->getLocalPlayer()->getSeat()))
                continue;

            const Ogre::Vector3& entityPos = entity->getPosition();
            double dist = Pathfinding::squaredDistance(entityPos.x, inputManager.mKeeperHandPos.x, entityPos.y, inputManager.mKeeperHandPos.y);
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
        // Stop creating rooms, traps, etc.
        inputManager.mLStartDragX = inputManager.mXPos;
        inputManager.mLStartDragY = inputManager.mYPos;
        unselectAllTiles();
        TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "");
        // If we have a currently selected action, we cancel it and don't try to slap or
        // drop what we have in hand
        if(mPlayerSelection.getCurrentAction() != SelectedAction::none)
        {
            mPlayerSelection.setCurrentAction(SelectedAction::none);
            return true;
        }

        if(mGameMap->getLocalPlayer()->numObjectsInHand() > 0)
        {
            // If we right clicked with the mouse over a valid map tile, try to drop what we have in hand on the map.
            Tile *curTile = mGameMap->getTile(inputManager.mKeeperHandGroundPos.x, inputManager.mKeeperHandGroundPos.y);

            if (curTile == nullptr)
                return true;

            if (mGameMap->getLocalPlayer()->isDropHandPossible(curTile))
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
            std::vector<GameEntity*> entities;
            tileClicked->fillWithEntities(entities, SelectionEntityWanted::any, mGameMap->getLocalPlayer());
            // We search the closest creature alive
            GameEntity* closestEntity = nullptr;
            double closestDist = 0;
            for(GameEntity* entity : entities)
            {
                if(!entity->canSlap(mGameMap->getLocalPlayer()->getSeat()))
                    continue;

                const Ogre::Vector3& entityPos = entity->getPosition();
                double dist = Pathfinding::squaredDistance(entityPos.x, inputManager.mKeeperHandGroundPos.x, entityPos.y, inputManager.mKeeperHandGroundPos.y);
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
    if (mPlayerSelection.getCurrentAction() == SelectedAction::none)
    {
        // See if the mouse is over any pickup-able entity
        std::vector<GameEntity*> entities;
        tileClicked->fillWithEntities(entities, SelectionEntityWanted::any, mGameMap->getLocalPlayer());
        // We search the closest creature alive
        GameEntity* closestEntity = nullptr;
        double closestDist = 0;
        for(GameEntity* entity : entities)
        {
            if(!entity->tryPickup(mGameMap->getLocalPlayer()->getSeat()))
                continue;

            const Ogre::Vector3& entityPos = entity->getPosition();
            double dist = Pathfinding::squaredDistance(entityPos.x, inputManager.mKeeperHandGroundPos.x, entityPos.y, inputManager.mKeeperHandGroundPos.y);
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
    }


    // If we are doing nothing and we click on a tile, it is a tile selection
    if(mPlayerSelection.getCurrentAction() == SelectedAction::none)
        mPlayerSelection.setCurrentAction(SelectedAction::selectTile);

    // If we are in a game we store the opposite of whether this tile is marked for digging or not, this allows us to mark tiles
    // by dragging out a selection starting from an unmarcked tile, or unmark them by starting the drag from a marked one.
    mDigSetBool = !(tileClicked->getMarkedForDigging(mGameMap->getLocalPlayer()));

    return true;
}

bool GameMode::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(Gui::convertButton(id));

    InputManager& inputManager = mModeManager->getInputManager();
    inputManager.mCommandState = InputCommandState::validated;

    // First check for the axis rotation release, as this
    // seems to be the most expected action the user wants to put at end

    if(id == OIS::MB_Middle)
    {
        inputManager.mMMouseDown = false;
        ODFrameListener::getSingleton().moveCamera(CameraManager::zeroRandomRotateX, 0.0);
        ODFrameListener::getSingleton().moveCamera(CameraManager::zeroRandomRotateY, 0.0);
    }

    // If the mouse press was on a CEGUI window ignore it
    if (inputManager.mMouseDownOnCEGUIWindow)
        return true;

    if (!isConnected())
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

bool GameMode::keyPressed(const OIS::KeyEvent& arg)
{
    // Inject key to Gui
    CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyDown(static_cast<CEGUI::Key::Scan>(arg.key));
    if (arg.text != 0)
    {
        CEGUI::System::getSingleton().getDefaultGUIContext().injectChar(arg.text);
    }

    switch (mCurrentInputMode)
    {
        case InputModeChat:
            return keyPressedChat(arg);
        case InputModeConsole:
            return getConsole()->keyPressed(arg);
        case InputModeNormal:
        default:
            return keyPressedNormal(arg);
    }
}

bool GameMode::keyPressedNormal(const OIS::KeyEvent &arg)
{
    ODFrameListener& frameListener = ODFrameListener::getSingleton();

    switch (arg.key)
    {
    case OIS::KC_F1:
        toggleHelpWindow();
        break;

    case OIS::KC_F2:
        togglePlayerSettingsWindow();
        break;

    case OIS::KC_F3:
        toggleObjectivesWindow();
        break;

    case OIS::KC_F4:
        toggleSkillWindow();
        break;

    case OIS::KC_F5:
        saveGame();
        break;

    case OIS::KC_F9:
        RenderManager::getSingleton().rrToggleHandSelectorVisibility();
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
        directionKeyPressed = true;
        break;

    case OIS::KC_RIGHT:
    case OIS::KC_D:
        frameListener.moveCamera(CameraManager::Direction::moveRight);
        directionKeyPressed = true;        
        break;

    case OIS::KC_UP:
    case OIS::KC_W:
        frameListener.moveCamera(CameraManager::Direction::moveForward);
        directionKeyPressed = true;
        break;

    case OIS::KC_DOWN:
    case OIS::KC_S:
        frameListener.moveCamera(CameraManager::Direction::moveBackward);
        directionKeyPressed = true;
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

    case OIS::KC_V:
        ODFrameListener::getSingleton().getCameraManager()->setNextDefaultView();
        break;

    case OIS::KC_LMENU:
        RenderManager::getSingleton().rrSetCreaturesTextOverlay(*mGameMap, true);
        break;

    // Zooms to the next event
    case OIS::KC_SPACE:
    {
        Player* player = mGameMap->getLocalPlayer();
        const PlayerEvent* event = player->getNextEvent(mIndexEvent);
        if(event == nullptr)
            break;

        Ogre::Vector3 pos = player->getSeat()->getStartingPosition();
        pos.x = static_cast<Ogre::Real>(event->getTile()->getX());
        pos.y = static_cast<Ogre::Real>(event->getTile()->getY());
        frameListener.cameraFlyTo(pos);
        break;
    }

    // Quit the game
    case OIS::KC_ESCAPE:
        popupExit(!mGameMap->getGamePaused());
        break;

    // Print a screenshot
    case OIS::KC_SYSRQ:
        ResourceManager::getSingleton().takeScreenshot(frameListener.getRenderWindow());
        break;

    case OIS::KC_RETURN:
    case OIS::KC_NUMPADENTER: {
        mCurrentInputMode = InputModeChat;
        CEGUI::Window* chatEditBox = mRootWindow->getChild("GameChatWindow/GameChatEditBox");
        chatEditBox->show();
        chatEditBox->activate();
        mChatMessageBoxDisplay |= ChatMessageBoxDisplay::showChatInput;
        refreshChatDisplay();
        break;
    }

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
    // If one presses Escape while in chat mode, let's simply quit it.
    CEGUI::Window* chatEditBox = mRootWindow->getChild("GameChatWindow/GameChatEditBox");
    if (arg.key == OIS::KC_ESCAPE)
    {
        mCurrentInputMode = InputModeNormal;
        chatEditBox->setText("");
        chatEditBox->hide();
        mChatMessageBoxDisplay &= ~ChatMessageBoxDisplay::showChatInput;
        refreshChatDisplay();
        return true;
    }

    if(arg.key != OIS::KC_RETURN && arg.key != OIS::KC_NUMPADENTER)
        return true;

    mCurrentInputMode = InputModeNormal;
    chatEditBox->hide();
    mChatMessageBoxDisplay &= ~ChatMessageBoxDisplay::showChatInput;
    refreshChatDisplay();

    // Check whether something was actually typed.
    if (chatEditBox->getText().empty())
        return true;

    ODClient::getSingleton().queueClientNotification(ClientNotificationType::chat, chatEditBox->getText().c_str());
    chatEditBox->setText("");
    return true;
}

void GameMode::refreshMainUI()
{
    Seat* mySeat = mGameMap->getLocalPlayer()->getSeat();
    CEGUI::Window* guiSheet = mRootWindow;

    //! \brief Updates common info on screen.
    CEGUI::Window* widget = guiSheet->getChild(Gui::DISPLAY_TERRITORY);
    std::stringstream tempSS("");
    tempSS << mySeat->getNumClaimedTiles();
    widget->setText(tempSS.str());

    widget = guiSheet->getChild(Gui::DISPLAY_CREATURES);
    tempSS.str("");
    tempSS << mySeat->getNumCreaturesFighters() << "/" << mySeat->getNumCreaturesFightersMax();
    widget->setText(tempSS.str());

    widget = guiSheet->getChild(Gui::DISPLAY_GOLD);
    tempSS.str("");
    tempSS << mySeat->getGold() << "/" << mySeat->getGoldMax();
    widget->setText(tempSS.str());

    widget = guiSheet->getChild(Gui::DISPLAY_MANA);
    tempSS.str("");
    tempSS << mySeat->getMana() << " " << (mySeat->getManaDelta() >= 0 ? "+" : "-")
            << mySeat->getManaDelta();
    widget->setText(tempSS.str());
}

void GameMode::refreshPlayerGoals(const std::string& goalsDisplayString)
{
    CEGUI::Window* widget = mRootWindow->getChild(Gui::OBJECTIVE_TEXT);
    widget->setText(reinterpret_cast<const CEGUI::utf8*>(goalsDisplayString.c_str()));
}

bool GameMode::keyReleased(const OIS::KeyEvent &arg)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyUp(static_cast<CEGUI::Key::Scan>(arg.key));

    if (mCurrentInputMode == InputModeChat || mCurrentInputMode == InputModeConsole)
        return true;

    return keyReleasedNormal(arg);
}

bool GameMode::keyReleasedNormal(const OIS::KeyEvent &arg)
{
    ODFrameListener& frameListener = ODFrameListener::getSingleton();

    switch (arg.key)
    {
    case OIS::KC_LEFT:
    case OIS::KC_A:
        frameListener.moveCamera(CameraManager::Direction::stopLeft);
        directionKeyPressed = false;
        break;

    case OIS::KC_RIGHT:
    case OIS::KC_D:
        frameListener.moveCamera(CameraManager::Direction::stopRight);
        directionKeyPressed = false;
        break;

    case OIS::KC_UP:
    case OIS::KC_W:
        frameListener.moveCamera(CameraManager::Direction::stopForward);
        directionKeyPressed = false;       
        break;

    case OIS::KC_DOWN:
    case OIS::KC_S:
        frameListener.moveCamera(CameraManager::Direction::stopBackward);
        directionKeyPressed = false;
        break;

    case OIS::KC_Q:
        frameListener.moveCamera(CameraManager::Direction::stopRotLeft);
        break;

    case OIS::KC_E:
        frameListener.moveCamera(CameraManager::Direction::stopRotRight);
        break;

    case OIS::KC_HOME:
        frameListener.moveCamera(CameraManager::Direction::stopDown);
        break;

    case OIS::KC_END:
        frameListener.moveCamera(CameraManager::Direction::stopUp);
        break;

    case OIS::KC_PGUP:
        frameListener.moveCamera(CameraManager::Direction::stopRotUp);
        break;

    case OIS::KC_PGDOWN:
        frameListener.moveCamera(CameraManager::Direction::stopRotDown);
        break;

    case OIS::KC_LMENU:
        RenderManager::getSingleton().rrSetCreaturesTextOverlay(*mGameMap, false);
        break;

    default:
        break;
    }

    return true;
}

void GameMode::handleHotkeys(OIS::KeyCode keycode)
{
    ODFrameListener& frameListener = ODFrameListener::getSingleton();
    InputManager& inputManager = mModeManager->getInputManager();

    //keycode minus two because the codes are shifted by two against the actual number
    unsigned int keynumber = keycode - 2;

    if (getKeyboard()->isModifierDown(OIS::Keyboard::Shift))
    {
        inputManager.mHotkeyLocationIsValid[keynumber] = true;
        inputManager.mHotkeyLocation[keynumber] = frameListener.getCameraViewTarget();
    }
    else if (inputManager.mHotkeyLocationIsValid[keynumber])
    {
        frameListener.cameraFlyTo(inputManager.mHotkeyLocation[keynumber]);
    }
}

void GameMode::onFrameStarted(const Ogre::FrameEvent& evt)
{
    GameEditorModeBase::onFrameStarted(evt);

    refreshGuiSkill();
    refreshSpellButtonCoolDowns();

    Player* player = mGameMap->getLocalPlayer();
    if (player == nullptr)
    {
        OD_LOG_ERR("No local player");
        return;
    }
    player->frameStarted(evt.timeSinceLastFrame);

    if((mSkillCurrentCompletion.mProgressBar != nullptr) &&
       (mSkillCurrentCompletion.mCompletenessDisplayed < mSkillCurrentCompletion.mCompleteness))
    {
        const float completeTime = 3.0f;
        // We update current skill completeness. Because the required amount of skill
        // may be pretty high between the lowest and highest skills, we cannot fill it
        // linearly according to this amount (it would be too fast for cheap skills or
        // too slow for expensive ones).
        // We want to fill it so that a 100% skill is filled in 'completeTime' seconds
        float progress = evt.timeSinceLastFrame / completeTime;
        mSkillCurrentCompletion.mCompletenessDisplayed = std::min(mSkillCurrentCompletion.mCompleteness,
            mSkillCurrentCompletion.mCompletenessDisplayed + progress);

        mSkillCurrentCompletion.mProgressBar->setProgress(mSkillCurrentCompletion.mCompletenessDisplayed);
    }
}

void GameMode::onFrameEnded(const Ogre::FrameEvent& evt)
{
}

void GameMode::popupExit(bool pause)
{
    if(pause)
    {
        mRootWindow->getChild(Gui::EXIT_CONFIRMATION_POPUP)->show();
    }
    else
    {
        mRootWindow->getChild(Gui::EXIT_CONFIRMATION_POPUP)->hide();
    }
    mGameMap->setGamePaused(pause);
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
                    ClientNotificationType::askPickupWorker);
                ODClient::getSingleton().queueClientNotification(clientNotification);
            }
            break;
        }
        case GuiAction::ButtonPressedCreatureFighter:
        {
            if(ODClient::getSingleton().isConnected())
            {
                ClientNotification *clientNotification = new ClientNotification(
                    ClientNotificationType::askPickupFighter);
                ODClient::getSingleton().queueClientNotification(clientNotification);
            }
            break;
        }
        default:
            break;
    }
}

bool GameMode::onClickYesQuitMenu(const CEGUI::EventArgs& /*arg*/)
{
    mModeManager->requestMode(AbstractModeManager::MENU_MAIN);
    return true;
}

bool GameMode::showObjectivesWindow(const CEGUI::EventArgs&)
{
    mRootWindow->getChild("ObjectivesWindow")->show();
    return true;
}

bool GameMode::hideObjectivesWindow(const CEGUI::EventArgs&)
{
    mRootWindow->getChild("ObjectivesWindow")->hide();
    return true;
}

bool GameMode::toggleObjectivesWindow(const CEGUI::EventArgs& e)
{
    CEGUI::Window* objectives = mRootWindow->getChild("ObjectivesWindow");

    if (objectives->isVisible())
        hideObjectivesWindow(e);
    else
        showObjectivesWindow(e);
    return true;
}

bool GameMode::showPlayerSettingsWindow(const CEGUI::EventArgs&)
{
    // Before showing the player settings, we reset to the values in the seat. That's
    // because only the server can change them and the values in the Seat are the
    // ones that should be shown
    syncPlayerSettings();
    mRootWindow->getChild("PlayerSettingsWindow")->show();
    return true;
}

bool GameMode::togglePlayerSettingsWindow(const CEGUI::EventArgs& e)
{
    CEGUI::Window* playerSettings = mRootWindow->getChild("PlayerSettingsWindow");

    if (playerSettings->isVisible())
        cancelPlayerSettings(e);
    else
        showPlayerSettingsWindow(e);
    return true;
}

bool GameMode::cancelPlayerSettings(const CEGUI::EventArgs&)
{
    mRootWindow->getChild("PlayerSettingsWindow")->hide();
    return true;
}

bool GameMode::applyPlayerSettings(const CEGUI::EventArgs&)
{
    mRootWindow->getChild("PlayerSettingsWindow")->hide();
    ClientNotification* clientNotification = new ClientNotification(
        ClientNotificationType::askSetPlayerSettings);

    CEGUI::ToggleButton* cbKoCreatures = static_cast<CEGUI::ToggleButton*>(
        mRootWindow->getChild("PlayerSettingsWindow/KOCreatureCheckbox"));
    bool enabled = cbKoCreatures->isSelected();
    clientNotification->mPacket << enabled;

    ODClient::getSingleton().queueClientNotification(clientNotification);

    return true;
}

void GameMode::syncPlayerSettings()
{
    Seat* localPlayerSeat = mGameMap->getLocalPlayer()->getSeat();
    CEGUI::ToggleButton* cbKoCreatures = static_cast<CEGUI::ToggleButton*>(
        mRootWindow->getChild("PlayerSettingsWindow/KOCreatureCheckbox"));
    cbKoCreatures->setSelected(localPlayerSeat->getKoCreatures());
}

bool GameMode::showSkillWindow(const CEGUI::EventArgs&)
{
    resetSkillTree();
    mRootWindow->getChild("SkillTreeWindow")->show();
    return true;
}

bool GameMode::hideSkillWindow(const CEGUI::EventArgs&)
{
    closeSkillWindow(false);
    return true;
}

bool GameMode::unselectAllSkillWindow(const CEGUI::EventArgs&)
{
    mSkillPending.clear();
    mSkillCurrentCompletion.resetValue();
    refreshGuiSkill(true);
    return true;
}

bool GameMode::applySkillWindow(const CEGUI::EventArgs& e)
{
    closeSkillWindow(true);
    return true;
}

void GameMode::closeSkillWindow(bool saveSkill)
{
    endSkillTree(saveSkill);
    mRootWindow->getChild("SkillTreeWindow")->hide();
}

bool GameMode::toggleSkillWindow(const CEGUI::EventArgs& e)
{
    CEGUI::Window* skill = mRootWindow->getChild("SkillTreeWindow");

    if (skill->isVisible())
    {
        closeSkillWindow(false);
    }
    else
    {
        showSkillWindow(e);
    }
    return true;
}

bool GameMode::showOptionsWindow(const CEGUI::EventArgs&)
{
    mRootWindow->getChild("GameOptionsWindow")->show();
    return true;
}

bool GameMode::hideOptionsWindow(const CEGUI::EventArgs& /*e*/)
{
    mRootWindow->getChild("GameOptionsWindow")->hide();
    return true;
}

bool GameMode::toggleOptionsWindow(const CEGUI::EventArgs& e)
{
    CEGUI::Window* options = mRootWindow->getChild("GameOptionsWindow");

    if (options->isVisible())
        hideOptionsWindow(e);
    else
        showOptionsWindow(e);
    return true;
}

bool GameMode::showQuitMenuFromOptions(const CEGUI::EventArgs& /*e*/)
{
    mRootWindow->getChild("GameOptionsWindow")->hide();
    popupExit(!mGameMap->getGamePaused());
    return true;
}

bool GameMode::showObjectivesFromOptions(const CEGUI::EventArgs& /*e*/)
{
    mRootWindow->getChild("GameOptionsWindow")->hide();
    mRootWindow->getChild("ObjectivesWindow")->show();
    return true;
}

bool GameMode::showSkillFromOptions(const CEGUI::EventArgs& /*e*/)
{
    mRootWindow->getChild("GameOptionsWindow")->hide();
    showSkillWindow();
    return true;
}

bool GameMode::saveGame(const CEGUI::EventArgs& /*e*/)
{
    // We can save if launching in server mode only
    if(!ODServer::getSingleton().isConnected())
    {
        std::string msg = "You cannot save the game when not launching in server mode";
        EventMessage* event = new EventMessage(msg, EventShortNoticeType::genericGameInfo);
        receiveEventShortNotice(event);
        return true;
    }

    if(ODClient::getSingleton().isConnected())
    {
        // Send a message to the server telling it we want to drop the creature
        ClientNotification *clientNotification = new ClientNotification(
            ClientNotificationType::askSaveMap);
        ODClient::getSingleton().queueClientNotification(clientNotification);
    }
    return true;
}

bool GameMode::showSettingsFromOptions(const CEGUI::EventArgs& /*e*/)
{
    mRootWindow->getChild("GameOptionsWindow")->hide();
    mSettings.show();
    return true;
}

bool GameMode::showHelpWindow(const CEGUI::EventArgs&)
{
    mRootWindow->getChild("GameHelpWindow")->show();
    return true;
}

bool GameMode::hideHelpWindow(const CEGUI::EventArgs& /*e*/)
{
    mRootWindow->getChild("GameHelpWindow")->hide();
    return true;
}

bool GameMode::toggleHelpWindow(const CEGUI::EventArgs& e)
{
    CEGUI::Window* helpWindow = mRootWindow->getChild("GameHelpWindow");
    if (!helpWindow->isVisible())
        showHelpWindow(e);
    else
        hideHelpWindow(e);
    return true;
}

void GameMode::setHelpWindowText()
{
    CEGUI::Window* textWindow = mRootWindow->getChild("GameHelpWindow/TextDisplay");
    const std::string formatTitleOn = "[font='MedievalSharp-12'][colour='CCBBBBFF']";
    const std::string formatTitleOff = "[font='MedievalSharp-10'][colour='FFFFFFFF']";
    std::stringstream txt("");
    txt << "Always skipping classes to play with kobolds in the torture room, were you?" << std::endl
        << "Well, at least you don't seem as dumb as a pit demon, so let's cover the basics again." << std::endl << std::endl;
    txt << formatTitleOn << "Camera controls" << formatTitleOff << std::endl
        << "The camera is your eyes - be watchful! Take the time to learn how to use it efficiently:" << std::endl
        << "  - Camera translation: Arrow keys or WASD." << std::endl
        << "  - Camera rotation: A (left) or E (right)." << std::endl
        << "  - Camera zooming: Mouse wheel, Home (zoom out) or End (zoom in)." << std::endl
        << "  - Camera tilting: Page Up (look up), Page Down (look down)." << std::endl
        << "  - Cycle through camera modes: V." << std::endl << std::endl;
    txt << formatTitleOn << "Keeper hand controls" << formatTitleOff << std::endl
        << "Use your hand wisely to keep all minions under control and let your dungeon thrive!" << std::endl
        << "  - Mouse left click: Select an action, Confirm an action." << std::endl
        << "  - Mouse right click: Unselect an action, Slap a creature (grin)." << std::endl
        << "  - Mouse middle click (on a creature): Show debugging information." << std::endl
        << "You can select multiple tiles for some actions by left clicking and dragging the mouse." << std::endl << std::endl;
    txt << formatTitleOn << "Basic workflow" << formatTitleOff << std::endl
        << "As an overlord of the underworld, your evil plan is to build a strong dungeon and to crush your neighbours."
        << "To do so, you need to deploy your shadow arts to their full extent:" << std::endl
        << "  - Summon workers to do the dirty job: dirt digging and gold mining. Workers will also claim ground "
        << "and wall tiles for your glory when they have nothing better to do. You can order workers around by marking "
        << "dirt and gold tiles for digging and mining." << std::endl
        << "  - Build rooms on claimed tiles. Building rooms costs gold, so make sure to have at least one treasury tile "
        << "in place (thanks to your evil tricks, the first tile is free!) so that your workers can store the gold they mine." << std::endl
        << "Be sure to build a dormitory and a hatchery to fulfill your creatures' lowest needs, and a library to skill "
        << "new buildings, spells and traps. Varied buildings, wealth and great dungeons will attract more powerful creatures." << std::endl
        << "  - Use the Skill Manager (F4) to set the priority for the various skills that can be uncovered at the library."
        << "Make sure to have intelligent creatures always at work at your library - if you can find any among your dumb minions!" << std::endl
        << "  - Once you have a workshop, set traps to protect your dungeon - your creatures will then craft them at the workshop." << std::endl
        << "  - Use spells to macro-manage your creatures more efficiently." << std::endl << std::endl;
    txt << formatTitleOn << "Tips and tricks" << formatTitleOff << std::endl
        << "You can left-click on one of your creatures to pick it up and right click somewhere else to put it back. "
        << "Very useful to help a creature in battle or force a worker to do a specific task..." << std::endl
        << "Note that you can place workers on any of your claimed tiles and unclaimed dirt tiles, "
        << "but you can only place fighters on allied claimed tiles and nothing at all on enemy claimed tiles." << std::endl
        << "Your workers will fortify walls, turning them into your color. Those cannot be broken by enemies until no more "
        << "claimed tiles around are of your color." << std::endl;
    txt << std::endl << std::endl << "Be evil, be cunning, your opponents will do the same... and have fun! ;)" << std::endl;
    textWindow->setText(reinterpret_cast<const CEGUI::utf8*>(txt.str().c_str()));
}

void GameMode::refreshSkillButtonState(const std::string& skillButtonName, const std::string& castButtonName,
        const std::string& skillProgressBarName, SkillType resType)
{
    // Determine the widget name and button accordingly to the SkillType given
    Seat* localPlayerSeat = mGameMap->getLocalPlayer()->getSeat();
    bool isDone = localPlayerSeat->isSkillDone(resType);
    bool isAllowed = true;
    uint32_t queueNumber = 0;
    if(!isDone)
    {
        const std::vector<SkillType>& skillNotAllowed = localPlayerSeat->getSkillNotAllowed();
        if(std::find(skillNotAllowed.begin(), skillNotAllowed.end(), resType) != skillNotAllowed.end())
        {
            // Skill is not allowed
            isAllowed = false;
        }
        else
        {
            // If we are currently changing the skill window, we display the temporary
            // modified pending list
            if(mIsSkillWindowOpen)
            {
                uint32_t cpt = 1;
                for (SkillType pendingRes : mSkillPending)
                {
                    if (pendingRes == resType)
                    {
                        queueNumber = cpt;
                        break;
                    }
                    ++cpt;
                }
            }
            else
            {
                queueNumber = localPlayerSeat->isSkillPending(resType);
            }
        }
    }

    const std::string okIcon = "OpenDungeonsIcons/CheckIcon";
    const std::string pendingIcon = "OpenDungeonsIcons/HourglassIcon";
    const std::string abortIcon = "OpenDungeonsIcons/AbortIcon";
    const std::string workIcon = "OpenDungeonsIcons/CogIcon";

    float curSkillProgress;
    SkillType curResType;
    if(!localPlayerSeat->getCurrentSkillProgress(curResType, curSkillProgress))
    {
        curResType = SkillType::nullSkillType;
    }

    // We show/hide the icons depending on available skills
    CEGUI::Window* guiSheet = mRootWindow;
    CEGUI::Window* skillsWindow = guiSheet->getChild("SkillTreeWindow/Skills");

    CEGUI::Window* skillButton = skillsWindow->getChild(skillButtonName);
    CEGUI::ProgressBar* skillProgressBar =
        static_cast<CEGUI::ProgressBar*>(skillsWindow->getChild(skillProgressBarName));
    if(isDone)
    {
        guiSheet->getChild(castButtonName)->show();
        skillButton->setText("");
        skillButton->setProperty("StateImage", okIcon);
        skillButton->setProperty("StateImageColour", "FF00BB00");
        skillButton->setEnabled(false);
        skillProgressBar->hide();
    }
    else if(!isAllowed)
    {
        guiSheet->getChild(castButtonName)->show();
        skillButton->setText("");
        skillButton->setProperty("StateImage", abortIcon);
        skillButton->setProperty("StateImageColour", "FFBB0000");
        skillButton->setEnabled(false);
        skillProgressBar->hide();
    }
    else if(resType == curResType)
    {
        // The skill is not available but skill is being done
        guiSheet->getChild(castButtonName)->hide();
        if(queueNumber == 0)
            skillButton->setText("");
        else
            skillButton->setText(Helper::toString(queueNumber));

        skillButton->setProperty("StateImage", workIcon);
        skillButton->setProperty("StateImageColour", "FF888800");
        skillButton->setEnabled(true);
        if (curSkillProgress > 0.0f)
        {
            // We reload the values is the current skill changed or if its value changed
            skillProgressBar->show();
            if((mSkillCurrentCompletion.mProgressBar != skillProgressBar) ||
               (mSkillCurrentCompletion.mCompleteness != curSkillProgress))
            {
                mSkillCurrentCompletion.setValue(skillProgressBar, curSkillProgress);
                skillProgressBar->setProgress(0);
            }
        }
        else
        {
            skillProgressBar->hide();
        }
    }
    else if (queueNumber >= 1)
    {
        // The skill is not available but skill is pending
        guiSheet->getChild(castButtonName)->hide();
        skillButton->setText(Helper::toString(queueNumber));
        skillButton->setProperty("StateImage", pendingIcon);
        skillButton->setProperty("StateImageColour", "FFFFFFFF");
        skillButton->setEnabled(true);
        skillProgressBar->hide();
    }
    else
    {
        // The skill is not available and skill is not pending
        guiSheet->getChild(castButtonName)->hide();
        skillButton->setText("");
        skillButton->setProperty("StateImage", "");
        skillButton->setEnabled(true);
        skillProgressBar->hide();
    }
}

void GameMode::refreshGuiSkill(bool forceRefresh)
{
    Seat* localPlayerSeat = mGameMap->getLocalPlayer()->getSeat();

    // If the percentage or the pending skill changed, we force refresh
    float curSkillProgress;
    SkillType curResType;
    if(localPlayerSeat->getCurrentSkillProgress(curResType, curSkillProgress) &&
       ((mCurrentSkillType != curResType) ||
        (mCurrentSkillProgress != curSkillProgress)))
    {
        forceRefresh = true;
    }

    if(!forceRefresh && !localPlayerSeat->getGuiSkillNeedsRefresh())
        return;

    if(mIsSkillWindowOpen && localPlayerSeat->getGuiSkillNeedsRefresh())
    {
        // We check if the temporary current pending list changed.
        for(auto it = mSkillPending.begin(); it != mSkillPending.end();)
        {
            SkillType resType = *it;
            if(!localPlayerSeat->isSkillDone(resType))
            {
                ++it;
                continue;
            }

            it = mSkillPending.erase(it);
        }
    }

    localPlayerSeat->guiSkillRefreshed();

    // We show/hide each icon depending on available skills
    SkillManager::listAllSkills([this](const std::string& skillButtonName, const std::string& castButtonName,
        const std::string& skillProgressBarName, SkillType resType)
    {
        refreshSkillButtonState(skillButtonName, castButtonName, skillProgressBarName, resType);
    });
}

void GameMode::refreshSpellButtonCoolDowns()
{
    Player* player = mGameMap->getLocalPlayer();
    if (player == nullptr)
    {
        OD_LOG_ERR("No local player");
        return;
    }

    // We show/hide each icon depending on available skills
    SkillManager::listAllSpellsProgressBars([this, player](SpellType spellType, const std::string& castProgressBarName)
    {
        CEGUI::ProgressBar* progressBar = static_cast<CEGUI::ProgressBar*>(mRootWindow->getChild(castProgressBarName));
        float progress = player->getSpellCooldownSmooth(spellType);
        if (progress > 0.0)
        {
            progressBar->show();
            progressBar->setProgress(progress);
        }
        else
        {
            progressBar->hide();
        }
    });
}

void GameMode::selectSquaredTiles(int tileX1, int tileY1, int tileX2, int tileY2)
{
    // Loop over the tiles in the rectangular selection region and set their setSelected flag accordingly.
    std::vector<Tile*> affectedTiles = mGameMap->rectangularRegion(tileX1,
        tileY1, tileX2, tileY2);

    selectTiles(affectedTiles);
}

void GameMode::selectTiles(const std::vector<Tile*> tiles)
{
    unselectAllTiles();

    Player* player = mGameMap->getLocalPlayer();
    for(Tile* tile : tiles)
    {
        tile->setSelected(true, player);
    }
}

void GameMode::unselectAllTiles()
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

void GameMode::displayText(const Ogre::ColourValue& txtColour, const std::string& txt)
{
    TextRenderer& textRenderer = TextRenderer::getSingleton();
    textRenderer.setColor(ODApplication::POINTER_INFO_STRING, txtColour);
    textRenderer.setText(ODApplication::POINTER_INFO_STRING, txt);
}

void GameMode::checkInputCommand()
{
    // In the gamemode, by default, we select tiles
    const InputManager& inputManager = mModeManager->getInputManager();

    switch(mPlayerSelection.getCurrentAction())
    {
        case SelectedAction::none:
            handlePlayerActionNone();
            return;
        case SelectedAction::selectTile:
            handlePlayerActionSelectTile();
            return;
        case SelectedAction::buildRoom:
            RoomManager::checkBuildRoom(mGameMap, mPlayerSelection.getNewRoomType(), inputManager, *this);
            return;
        case SelectedAction::destroyRoom:
            RoomManager::checkSellRoomTiles(mGameMap, inputManager, *this);
            return;
        case SelectedAction::castSpell:
            SpellManager::checkSpellCast(mGameMap, mPlayerSelection.getNewSpellType(), inputManager, *this);
            return;
        case SelectedAction::buildTrap:
            TrapManager::checkBuildTrap(mGameMap, mPlayerSelection.getNewTrapType(), inputManager, *this);
            return;
        case SelectedAction::destroyTrap:
            TrapManager::checkSellTrapTiles(mGameMap, inputManager, *this);
            return;
        default:
            return;
    }
}

void GameMode::handlePlayerActionNone()
{
    const InputManager& inputManager = mModeManager->getInputManager();
    // We only display the selection cursor on the hovered tile
    if(inputManager.mCommandState == InputCommandState::validated)
    {
        unselectAllTiles();
        return;
    }
}

void GameMode::handlePlayerActionSelectTile()
{
    const InputManager& inputManager = mModeManager->getInputManager();
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos,
            inputManager.mYPos);
        return;
    }

    if(inputManager.mCommandState == InputCommandState::building)
    {
        selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mLStartDragX,
            inputManager.mLStartDragY);
        return;
    }

    unselectAllTiles();

    ClientNotification *clientNotification = new ClientNotification(
        ClientNotificationType::askMarkTiles);
    clientNotification->mPacket << inputManager.mXPos << inputManager.mYPos;
    clientNotification->mPacket << inputManager.mLStartDragX << inputManager.mLStartDragY;
    clientNotification->mPacket << mDigSetBool;
    ODClient::getSingleton().queueClientNotification(clientNotification);
    mPlayerSelection.setCurrentAction(SelectedAction::none);
}

void GameMode::resetSkillTree()
{
    mSkillPending = mGameMap->getLocalPlayer()->getSeat()->getSkillPending();
    mSkillCurrentCompletion.resetValue();
    mIsSkillWindowOpen = true;
}

bool GameMode::skillButtonTreeClicked(SkillType type)
{
    // If the skill is already done or not allowed, nothing to do
    const std::vector<SkillType>& skillDone = mGameMap->getLocalPlayer()->getSeat()->getSkillDone();
    if(std::find(skillDone.begin(), skillDone.end(), type) != skillDone.end())
        return false;
    const std::vector<SkillType>& skillNotAllowed = mGameMap->getLocalPlayer()->getSeat()->getSkillNotAllowed();
    if(std::find(skillNotAllowed.begin(), skillNotAllowed.end(), type) != skillNotAllowed.end())
        return false;

    auto it = std::find(mSkillPending.begin(), mSkillPending.end(), type);
    if(it != mSkillPending.end())
    {
        // The skill is pending. We remove it as well as all its dependencies
        mSkillPending.erase(it);

        for(it = mSkillPending.begin(); it != mSkillPending.end();)
        {
            SkillType pendingType = *it;
            const Skill* skill = SkillManager::getSkill(pendingType);
            if(skill == nullptr)
            {
                OD_LOG_ERR("null skill pendingType=" + Helper::toString(static_cast<uint32_t>(pendingType)));
                continue;
            }

            if(!skill->dependsOn(type))
            {
                ++it;
                continue;
            }
            it = mSkillPending.erase(it);
        }
        return true;
    }

    std::vector<SkillType> skillToAdd;

    // The skill is not pending. We need to check availability to all its dependencies and
    // add them at the end of the list if all are available/done
    const Skill* skill = SkillManager::getSkill(type);
    std::vector<SkillType> dependencies;
    skill->buildDependencies(skillDone, dependencies);

    // We check if one of the dependencies is not available. If not, we cannot skill
    for(SkillType skillType : dependencies)
    {
        if(std::find(skillNotAllowed.begin(), skillNotAllowed.end(), skillType) != skillNotAllowed.end())
            return false;

        if(std::find(mSkillPending.begin(), mSkillPending.end(), skillType) != mSkillPending.end())
            continue;

        skillToAdd.push_back(skillType);
    }

    for(SkillType skillType : skillToAdd)
        mSkillPending.push_back(skillType);

    return true;
}

bool GameMode::autoFillSkillWindow(const CEGUI::EventArgs&)
{
    SkillManager::buildRandomPendingSkillsForSeat(mSkillPending,
        mGameMap->getLocalPlayer()->getSeat());
    refreshGuiSkill(true);
    return true;
}

void GameMode::endSkillTree(bool apply)
{
    if(apply)
    {
        uint32_t nbItems = static_cast<uint32_t>(mSkillPending.size());
        ClientNotification *clientNotification = new ClientNotification(
            ClientNotificationType::askSetSkillTree);
        clientNotification->mPacket << nbItems;
        for(const SkillType& type : mSkillPending)
        {
            clientNotification->mPacket << type;
        }
        ODClient::getSingleton().queueClientNotification(clientNotification);
    }

    mSkillPending.clear();
    mSkillCurrentCompletion.resetValue();
    mIsSkillWindowOpen = false;
    refreshGuiSkill(true);
}

void GameMode::buildPlayerSettingsWindow()
{
    if(!mSeatIds.empty())
        return;

    CEGUI::Window* tmpWin = mRootWindow->getChild("PlayerSettingsWindow/Seats/SeatsSP");
    CEGUI::WindowManager& winMgr = CEGUI::WindowManager::getSingleton();
    float offset = 15;
    for(Seat* seat : mGameMap->getSeats())
    {
        if(seat->isRogueSeat())
            continue;

        Player* player = seat->getPlayer();
        if(player == nullptr)
            continue;

        CEGUI::DefaultWindow* tmpElt;
        std::string name;
        Ogre::ColourValue seatColor = seat->getColorValue();
        seatColor.a = 1.0f; // Restore the color opacity

        name = TEXT_SEAT_ID_PREFIX + Helper::toString(seat->getId());
        tmpElt = static_cast<CEGUI::DefaultWindow*>(winMgr.createWindow("OD/StaticText", name));
        tmpWin->addChild(tmpElt);
        tmpElt->setArea(CEGUI::UDim(0,20), CEGUI::UDim(0, offset), CEGUI::UDim(0.2,-20), CEGUI::UDim(0,offset));
        tmpElt->setText("[colour='" + Helper::getCEGUIColorFromOgreColourValue(seatColor) + "']" + Helper::toString(seat->getId()));
        tmpElt->setProperty("FrameEnabled", "False");
        tmpElt->setProperty("BackgroundEnabled", "False");

        name = TEXT_SEAT_PLAYER_NICKNAME_PREFIX + Helper::toString(seat->getId());
        tmpElt = static_cast<CEGUI::DefaultWindow*>(winMgr.createWindow("OD/StaticText", name));
        tmpWin->addChild(tmpElt);
        tmpElt->setArea(CEGUI::UDim(0.2,20), CEGUI::UDim(0, offset), CEGUI::UDim(0.6,-20), CEGUI::UDim(0,offset));
        tmpElt->setText("[colour='" + Helper::getCEGUIColorFromOgreColourValue(seatColor) + "']" + player->getNick());
        tmpElt->setProperty("FrameEnabled", "False");
        tmpElt->setProperty("BackgroundEnabled", "False");

        name = TEXT_SEAT_TEAM_ID_PREFIX + Helper::toString(seat->getId());
        tmpElt = static_cast<CEGUI::DefaultWindow*>(winMgr.createWindow("OD/StaticText", name));
        tmpWin->addChild(tmpElt);
        tmpElt->setArea(CEGUI::UDim(0.8,20), CEGUI::UDim(0, offset), CEGUI::UDim(0.2,-20), CEGUI::UDim(0,offset));
        tmpElt->setText("[colour='" + Helper::getCEGUIColorFromOgreColourValue(seatColor) + "']" + Helper::toString(seat->getTeamId()));
        tmpElt->setProperty("FrameEnabled", "False");
        tmpElt->setProperty("BackgroundEnabled", "False");

        mSeatIds.push_back(seat->getId());
        offset += 15;
    }
}
