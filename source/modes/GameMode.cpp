/*!
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

#include "modes/GameMode.h"

#include "modes/GameEditorModeConsole.h"

#include "ODApplication.h"
#include "camera/CameraManager.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "network/ODClient.h"
#include "network/ODServer.h"
#include "game/Player.h"
#include "game/Research.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "gamemap/Pathfinding.h"
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
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/ResourceManager.h"
#include "traps/Trap.h"
#include "traps/TrapManager.h"
#include "traps/TrapType.h"

#include <CEGUI/WindowManager.h>
#include <CEGUI/widgets/PushButton.h>
#include <CEGUI/widgets/ToggleButton.h>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>

#include <algorithm>
#include <vector>
#include <string>

namespace
{
    //! \brief Functor to select spell from gui
    class SpellSelector
    {
    public:
        bool operator()(const CEGUI::EventArgs& e)
        {
            playerSelection.setCurrentAction(SelectedAction::castSpell);
            playerSelection.setNewSpellType(spellType);
            return true;
        }
        SpellType spellType;
        PlayerSelection& playerSelection;
    };
}

GameMode::GameMode(ModeManager *modeManager):
    GameEditorModeBase(modeManager, ModeManager::GAME, modeManager->getGui().getGuiSheet(Gui::guiSheet::inGameMenu)),
    mDigSetBool(false),
    mIndexEvent(0)
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
        guiSheet->getChild("ObjectivesWindow/__auto_closebutton__")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::hideObjectivesWindow, this)
        )
    );

    // The research tree window
    addEventConnection(
        guiSheet->getChild("ResearchButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::toggleResearchWindow, this)
        )
    );
    addEventConnection(
        guiSheet->getChild("ResearchTreeWindow/__auto_closebutton__")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::hideResearchWindow, this)
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
        guiSheet->getChild("GameOptionsWindow/__auto_closebutton__")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
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
        guiSheet->getChild("GameOptionsWindow/ResearchButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::showResearchFromOptions, this)
        )
    );
    addEventConnection(
        guiSheet->getChild("GameOptionsWindow/SaveGameButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::saveGame, this)
        )
    );
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
    //Settings window
    addEventConnection(
        guiSheet->getChild("SettingsWindow/CancelButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::hideSettingsWindow, this)
        )
    );
    addEventConnection(
        guiSheet->getChild("SettingsWindow/__auto_closebutton__")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::hideSettingsWindow, this)
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
        guiSheet->getChild("ConfirmExit/__auto_closebutton__")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(cancelExitWindow)
        )
    );

    // Help window
    addEventConnection(
        guiSheet->getChild("GameHelpWindow/__auto_closebutton__")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GameMode::hideHelpWindow, this)
        )
    );

    // Set the help window text
    setHelpWindowText();

    //Spells
    connectSpellSelect(Gui::BUTTON_SPELL_CALLTOWAR, SpellType::callToWar);
    connectSpellSelect(Gui::BUTTON_SPELL_SUMMON_WORKER, SpellType::summonWorker);
    connectSpellSelect(Gui::BUTTON_SPELL_CREATURE_HEAL, SpellType::creatureHeal);
    connectSpellSelect(Gui::BUTTON_SPELL_CREATURE_EXPLOSION, SpellType::creatureExplosion);

    syncTabButtonTooltips(Gui::MAIN_TABCONTROL);
}

GameMode::~GameMode()
{
    CEGUI::ToggleButton* checkBox =
        dynamic_cast<CEGUI::ToggleButton*>(
            mRootWindow->getChild(
                Gui::EXIT_CONFIRMATION_POPUP)->getChild("SaveReplayCheckbox"));
    if(ODClient::getSingleton().isConnected())
        ODClient::getSingleton().disconnect(checkBox->isSelected());
    if(ODServer::getSingleton().isConnected())
        ODServer::getSingleton().stopServer();

    // Now that the server is stopped, we can clear the client game map
    ODFrameListener::getSingleton().getClientGameMap()->clearAll();
}

void GameMode::activate()
{
    // Loads the corresponding Gui sheet.
    Gui& gui = getModeManager().getGui();
    gui.loadGuiSheet(Gui::inGameMenu);

    // Hides the exit pop-up and certain buttons only used by the editor.
    CEGUI::Window* guiSheet = mRootWindow;
    guiSheet->getChild(Gui::EXIT_CONFIRMATION_POPUP)->hide();
    guiSheet->getChild("ObjectivesWindow")->hide();
    guiSheet->getChild("ResearchTreeWindow")->hide();
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
    refreshGuiResearch(true);
}

bool GameMode::mouseMoved(const OIS::MouseEvent &arg)
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
    ODFrameListener::getSingleton().findWorldPositionFromMouse(arg, keeperHandPos);
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

void GameMode::handleMouseWheel(const OIS::MouseEvent& arg)
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

bool GameMode::mousePressed(const OIS::MouseEvent& arg, OIS::MouseButtonID id)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
        Gui::convertButton(id));

    if (!isConnected())
        return true;

    InputManager& inputManager = mModeManager->getInputManager();
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

        // Stop creating rooms, traps, etc.
        inputManager.mLStartDragX = inputManager.mXPos;
        inputManager.mLStartDragY = inputManager.mYPos;
        unselectAllTiles();
        mPlayerSelection.setCurrentAction(SelectedAction::none);
        TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "");

        if(mGameMap->getLocalPlayer()->numObjectsInHand() > 0)
        {
            // If we right clicked with the mouse over a valid map tile, try to drop what we have in hand on the map.
            Tile *curTile = mGameMap->getTile(inputManager.mXPos, inputManager.mYPos);

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
    if (mPlayerSelection.getCurrentAction() == SelectedAction::none)
    {
        // See if the mouse is over any pickup-able entity
        std::vector<EntityBase*> entities;
        tileClicked->fillWithEntities(entities, SelectionEntityWanted::any, mGameMap->getLocalPlayer());
        // We search the closest creature alive
        EntityBase* closestEntity = nullptr;
        double closestDist = 0;
        for(EntityBase* entity : entities)
        {
            if(!entity->tryPickup(mGameMap->getLocalPlayer()->getSeat()))
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
    CEGUI::System::getSingleton().getDefaultGUIContext().injectChar(arg.text);

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

    case OIS::KC_F3:
        toggleObjectivesWindow();
        break;

    case OIS::KC_F4:
        toggleResearchWindow();
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
        return true;
    }

    if(arg.key != OIS::KC_RETURN && arg.key != OIS::KC_NUMPADENTER)
        return true;

    mCurrentInputMode = InputModeNormal;
    chatEditBox->hide();

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
    tempSS << mySeat->getGold();
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
    //Update the minimap
    //TODO: We should add some check to only update this if the camera has moved, or map changed
    CameraManager& cameraManager = *ODFrameListener::getSingleton().getCameraManager();
    mMiniMap.updateCameraInfo(cameraManager.getCameraViewTarget(),
                              cameraManager.getActiveCameraNode()->getOrientation().getRoll().valueRadians());
    mMiniMap.draw(*mGameMap);
    mMiniMap.swap();

    refreshGuiResearch();
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

bool GameMode::showResearchWindow(const CEGUI::EventArgs&)
{
    mRootWindow->getChild("ResearchTreeWindow")->show();
    return true;
}

bool GameMode::hideResearchWindow(const CEGUI::EventArgs&)
{
    mRootWindow->getChild("ResearchTreeWindow")->hide();
    return true;
}

bool GameMode::toggleResearchWindow(const CEGUI::EventArgs& e)
{
    CEGUI::Window* research = mRootWindow->getChild("ResearchTreeWindow");

    if (research->isVisible())
        hideResearchWindow(e);
    else
        showResearchWindow(e);
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

bool GameMode::hideSettingsWindow(const CEGUI::EventArgs&)
{
    mRootWindow->getChild("SettingsWindow")->hide();
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

bool GameMode::showResearchFromOptions(const CEGUI::EventArgs& /*e*/)
{
    mRootWindow->getChild("GameOptionsWindow")->hide();
    mRootWindow->getChild("ResearchTreeWindow")->show();
    return true;
}

bool GameMode::saveGame(const CEGUI::EventArgs& /*e*/)
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

bool GameMode::showSettingsFromOptions(const CEGUI::EventArgs& /*e*/)
{
    mRootWindow->getChild("GameOptionsWindow")->hide();
    mRootWindow->getChild("SettingsWindow")->show();
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
    txt << "Welcome to the OpenDungeons quick help!" << std::endl << std::endl
        << formatTitleOn << "Camera Controls" << formatTitleOff << std::endl
        << "Camera translation: Arrow keys or WASD." << std::endl
        << "Camera rotation: A (left) or E (right)." << std::endl
        << "Camera zooming: Mouse wheel, Home (zoom out) or End (zoom in)." << std::endl
        << "Camera tilting: Page Up (look up), Page Down (look down)." << std::endl << std::endl;
    txt << formatTitleOn << "Keeper Hand Controls" << formatTitleOff << std::endl
        << "Mouse left click: Select an action, Confirm an action." << std::endl
        << "Mouse right click: Unselect an action, Slap a creature (grin)." << std::endl
        << "Mouse middle click (on a creature): Show debugging information." << std::endl
        << "You can select multiple tiles for some actions by left clicking and dragging the mouse." << std::endl << std::endl;
    txt << formatTitleOn << "Basic gameplay" << formatTitleOff << std::endl
        << "As an overlord of the underworld, your evil plan is to build a strong dungeon and to crush your neighbours." << std::endl
        << "To do so, you need to deploy your shadow arts to their full extent:" << std::endl
        << "  - Summon workers to do the dirty job: dirt digging and gold mining. Workers will also claim ground "
        << "and wall tiles for your glory when they have nothing better to do. You can order workers around by marking "
        << "dirt and gold tiles for digging and mining." << std::endl
        << "  - Build rooms on claimed tiles. Building rooms costs gold, so make sure to have at least one treasury tile "
        << "in place (thanks to your evil tricks, the first tile is free!) so that your workers can store the gold they mine." << std::endl
        << "Be sure to build a dormitory and a hatchery to fulfill your creatures' lowest needs, and a library to research "
        << "new buildings, spells and traps. Varied buildings, wealth and great dungeons will attract more powerful creatures." << std::endl
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

void GameMode::refreshResearchButtonState(ResearchType resType)
{
    const std::string okIcon = "OpenDungeonsIcons/CheckIcon";
    const std::string flagIcon = "OpenDungeonsIcons/SeatIcon";
    const std::string inProgressIcon = "OpenDungeonsIcons/CogIcon";
    const std::string pendingIcon = "OpenDungeonsIcons/HourglassIcon";

    // The current pending research or nullResearch if none.
    Seat* localPlayerSeat = mGameMap->getLocalPlayer()->getSeat();

    // Determine the widget name and button accordingly to the ResearchType given
    std::string ceguiWidgetName;
    std::string ceguiWidgetButtonName;
    switch(resType)
    {
        case ResearchType::nullResearchType:
        default:
            return;
            break;
        case ResearchType::roomDormitory:
            ceguiWidgetName = "TacticSkills/DormitoryButton";
            ceguiWidgetButtonName = Gui::BUTTON_DORMITORY;
            break;
        case ResearchType::roomTreasury:
            ceguiWidgetName = "TacticSkills/TreasuryButton";
            ceguiWidgetButtonName = Gui::BUTTON_TREASURY;
            break;
        case ResearchType::roomHatchery:
            ceguiWidgetName = "TacticSkills/HatcheryButton";
            ceguiWidgetButtonName = Gui::BUTTON_HATCHERY;
            break;
        case ResearchType::roomWorkshop:
            ceguiWidgetName = "TacticSkills/WorkshopButton";
            ceguiWidgetButtonName = Gui::BUTTON_WORKSHOP;
            break;
        case ResearchType::trapCannon:
            ceguiWidgetName = "TacticSkills/CannonButton";
            ceguiWidgetButtonName = Gui::BUTTON_TRAP_CANNON;
            break;
        case ResearchType::trapSpike:
            ceguiWidgetName = "TacticSkills/SpikeTrapButton";
            ceguiWidgetButtonName = Gui::BUTTON_TRAP_SPIKE;
            break;
        case ResearchType::trapBoulder:
            ceguiWidgetName = "TacticSkills/BoulderTrapButton";
            ceguiWidgetButtonName = Gui::BUTTON_TRAP_BOULDER;
            break;
        case ResearchType::trapDoorWooden:
            ceguiWidgetName = "TacticSkills/WoodenDoorTrapButton";
            ceguiWidgetButtonName = Gui::BUTTON_TRAP_DOOR_WOODEN;
            break;
        case ResearchType::roomLibrary:
            ceguiWidgetName = "MagicSkills/LibraryButton";
            ceguiWidgetButtonName = Gui::BUTTON_LIBRARY;
            break;
        case ResearchType::roomCrypt:
            ceguiWidgetName = "MagicSkills/CryptButton";
            ceguiWidgetButtonName = Gui::BUTTON_CRYPT;
            break;
        case ResearchType::spellSummonWorker:
            ceguiWidgetName = "MagicSkills/SummonWorkerButton";
            ceguiWidgetButtonName = Gui::BUTTON_SPELL_SUMMON_WORKER;
            break;
        case ResearchType::roomTrainingHall:
            ceguiWidgetName = "AttackSkills/TrainingHallButton";
            ceguiWidgetButtonName = Gui::BUTTON_TRAININGHALL;
            break;
        case ResearchType::spellCallToWar:
            ceguiWidgetName = "AttackSkills/CallToWarButton";
            ceguiWidgetButtonName = Gui::BUTTON_SPELL_CALLTOWAR;
            break;
        case ResearchType::spellCreatureHeal:
            ceguiWidgetName = "MagicSkills/CreatureHealButton";
            ceguiWidgetButtonName = Gui::BUTTON_SPELL_CREATURE_HEAL;
            break;
        case ResearchType::spellCreatureExplosion:
            ceguiWidgetName = "AttackSkills/CreatureExplosionButton";
            ceguiWidgetButtonName = Gui::BUTTON_SPELL_CREATURE_EXPLOSION;
            break;
    }

    // We show/hide the icons depending on available researches
    CEGUI::Window* guiSheet = mRootWindow;
    CEGUI::Window* researchGui = guiSheet->getChild("ResearchTreeWindow");

    CEGUI::Window* researchButton = researchGui->getChild(ceguiWidgetName);
    uint32_t queueNumber = localPlayerSeat->isResearchPending(resType);
    if(localPlayerSeat->isResearchDone(resType))
    {
        guiSheet->getChild(ceguiWidgetButtonName)->show();
        researchButton->setText("");
        researchButton->setProperty("StateImage", okIcon);
        researchButton->setProperty("StateImageColour", "FF00BB00");
        researchButton->setEnabled(false);
    }
    else if (localPlayerSeat->hasResearchWaitingForType(resType))
    {
        researchButton->setText("");
        researchButton->setProperty("StateImage", flagIcon);
        researchButton->setProperty("StateImageColour", "FFBB0000");
        researchButton->setEnabled(false);
    }
    else if (localPlayerSeat->getCurrentResearchType() == resType)
    {
        researchButton->setText("");
        researchButton->setProperty("StateImage", inProgressIcon);
        researchButton->setProperty("StateImageColour", "FFBBBB00");
        researchButton->setEnabled(true);
    }
    else if (queueNumber > 0)
    {
        researchButton->setText(Helper::toString(queueNumber));
        researchButton->setProperty("StateImage", pendingIcon);
        researchButton->setProperty("StateImageColour", "FFFFFFFF");
        researchButton->setEnabled(true);
    }
    else
    {
        guiSheet->getChild(ceguiWidgetButtonName)->hide();
        researchButton->setText("");
        researchButton->setProperty("StateImage", "");
        researchButton->setEnabled(true);
    }
}

void GameMode::refreshGuiResearch(bool forceRefresh)
{
    Seat* localPlayerSeat = mGameMap->getLocalPlayer()->getSeat();
    if(!forceRefresh && !localPlayerSeat->getGuiResearchNeedsRefresh())
        return;

    localPlayerSeat->guiResearchRefreshed();

    // We show/hide each icon depending on available researches
    for (uint16_t i = static_cast<uint16_t>(ResearchType::nullResearchType);
            i < static_cast<uint16_t>(ResearchType::countResearch); ++i)
    {
        refreshResearchButtonState(static_cast<ResearchType>(i));
    }

}

void GameMode::connectSpellSelect(const std::string& buttonName, SpellType spellType)
{
    addEventConnection(
        mRootWindow->getChild(buttonName)->subscribeEvent(
          CEGUI::PushButton::EventClicked,
          CEGUI::Event::Subscriber(SpellSelector{spellType, mPlayerSelection})
        )
    );
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

    selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos,
        inputManager.mYPos);
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
