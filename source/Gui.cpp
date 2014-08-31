/*
 * \file   Gui.cpp
 * \date   05 April 2011
 * \author StefanP.MUC
 * \brief  Class Gui containing all the stuff for the GUI, including translation.
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

//TODO: The event handlers should be call functions to AS instead of hardcoded so that we can
//      script the GUI actions. Maybe even register CEGUI to AS to make it fully scripted?
//      Then we could easily adjust the GUI without recompiling.

#include "Gui.h"

#include "MapLoader.h"
#include "ODFrameListener.h"
#include "GameMap.h"
#include "Player.h"
#include "Trap.h"
#include "TextRenderer.h"
#include "ODApplication.h"
#include "ODServer.h"
#include "ODClient.h"
#include "CameraManager.h"
#include "MiniMap.h"
#include "ModeManager.h"
#include "GameMode.h"
#include "EditorMode.h"
#include "MenuModeSkirmish.h"
#include "MenuModeMultiplayer.h"
#include "LogManager.h"

#include <CEGUI/CEGUI.h>
#include <CEGUI/RendererModules/Ogre/Renderer.h>
#include <CEGUI/SchemeManager.h>
#include <CEGUI/System.h>
#include <CEGUI/WindowManager.h>
#include <CEGUI/widgets/PushButton.h>
#include <CEGUI/Event.h>

template<> Gui* Ogre::Singleton<Gui>::msSingleton = 0;

Gui::Gui()
{
    CEGUI::OgreRenderer::bootstrapSystem();

    CEGUI::SchemeManager::getSingleton().createFromFile("OpenDungeonsSkin.scheme");

    // CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().setImage("OpenDungeons/MouseArrow");
    // CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().setVisible(true);
    CEGUI::System::getSingleton().getDefaultGUIContext().setDefaultTooltipObject(new CEGUI::Tooltip("OD","Tooltip"));

    CEGUI::WindowManager* wmgr = CEGUI::WindowManager::getSingletonPtr();

    CEGUI::Window* myHide = wmgr->createWindow("DefaultWindow", "DummyWindow");
    sheets[hideGui] = myHide;

    sheets[inGameMenu] = wmgr->loadLayoutFromFile("OpenDungeons.layout");
    sheets[mainMenu] = wmgr->loadLayoutFromFile("OpenDungeonsMainMenu.layout");
    sheets[skirmishMenu] = wmgr->loadLayoutFromFile("OpenDungeonsMenuSkirmish.layout");
    sheets[multiplayerMenu] = wmgr->loadLayoutFromFile("OpenDungeonsMenuMultiplayer.layout");
    sheets[editorMenu] =  wmgr->loadLayoutFromFile("OpenDungeonsEditorMenu.layout");

    assignEventHandlers();
}

Gui::~Gui()
{
    //CEGUI::OgreRenderer::destroySystem();
}

CEGUI::MouseButton Gui::convertButton(const OIS::MouseButtonID& buttonID)
{
    switch (buttonID)
    {
    default:
    case OIS::MB_Left:
        return CEGUI::LeftButton;

    case OIS::MB_Right:
        return CEGUI::RightButton;

    case OIS::MB_Middle:
        return CEGUI::MiddleButton;
    }
}

void Gui::loadGuiSheet(const guiSheet& newSheet)
{
    CEGUI::System::getSingletonPtr()->getDefaultGUIContext().setRootWindow(sheets[newSheet]);
    // This shouldn't be needed, but the gui seems to not allways change when using hideGui without it.
    CEGUI::System::getSingletonPtr()->getDefaultGUIContext().markAsDirty();
}

CEGUI::Window* Gui::getGuiSheet(const guiSheet& sheet)
{
    if (sheets.find(sheet) != sheets.end())
    {
        return sheets[sheet];
    }
    return NULL;
}

void Gui::assignEventHandlers()
{
    //std::cout << "Gui::assignEventHandlers()" << std::endl;

    CEGUI::Window* rootWindow = sheets[inGameMenu];

    if (rootWindow == NULL)
    {
        LogManager::getSingleton().logMessage("Gui: No root window!");
        return;
    }

    // Main menu controls
    sheets[mainMenu]->getChild(MM_BUTTON_START_SKIRMISH)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&mMNewGameButtonPressed));

    sheets[mainMenu]->getChild(MM_BUTTON_START_MULTIPLAYER)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&mMNewGameMultiButtonPressed));

    sheets[mainMenu]->getChild(MM_BUTTON_MAPEDITOR)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&mMMapEditorButtonPressed));

    sheets[mainMenu]->getChild(MM_BUTTON_QUIT)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&mMQuitButtonPressed));

    // Game Mode controls
    sheets[inGameMenu]->getChild(BUTTON_DORMITORY)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&dormitoryButtonPressed));

    sheets[inGameMenu]->getChild(BUTTON_TREASURY)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&treasuryButtonPressed));

    sheets[inGameMenu]->getChild(BUTTON_FORGE)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&forgeButtonPressed));

    sheets[inGameMenu]->getChild(BUTTON_TRAININGHALL)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&trainingHallButtonPressed));

    sheets[inGameMenu]->getChild(BUTTON_LIBRARY)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&libraryButtonPressed));

    sheets[inGameMenu]->getChild(BUTTON_HATCHERY)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&hatcheryButtonPressed));

    sheets[inGameMenu]->getChild(BUTTON_CANNON)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&cannonButtonPressed));

    sheets[inGameMenu]->getChild(MINIMAP)->subscribeEvent(
            CEGUI:: Window::EventMouseClick,
            CEGUI::Event::Subscriber(&miniMapclicked));

    sheets[inGameMenu]->getChild(EXIT_CONFIRMATION_POPUP_YES_BUTTON)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&confirmExitYesButtonPressed));

    sheets[inGameMenu]->getChild(EXIT_CONFIRMATION_POPUP_NO_BUTTON)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&confirmExitNoButtonPressed));

    // Editor Mode controls
    sheets[editorMenu]->getChild(EDITOR_LAVA_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorLavaButtonPressed));

    sheets[editorMenu]->getChild(EDITOR_GOLD_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorGoldButtonPressed));

    sheets[editorMenu]->getChild(EDITOR_ROCK_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorRockButtonPressed));

    sheets[editorMenu]->getChild(EDITOR_WATER_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorWaterButtonPressed));

    sheets[editorMenu]->getChild(EDITOR_DIRT_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorDirtButtonPressed));

    sheets[editorMenu]->getChild(EDITOR_CLAIMED_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorClaimedButtonPressed));

    // Skirmish level select menu controls
    sheets[skirmishMenu]->getChild(SKM_BUTTON_LAUNCH)->subscribeEvent(
        CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&mSKMLoadButtonPressed));

    sheets[skirmishMenu]->getChild(SKM_BUTTON_BACK)->subscribeEvent(
        CEGUI::PushButton::EventClicked,
        CEGUI::Event::Subscriber(&mSKMBackButtonPressed));

    sheets[skirmishMenu]->getChild(SKM_LIST_LEVELS)->subscribeEvent(
        CEGUI::Listbox::EventMouseDoubleClick,
        CEGUI::Event::Subscriber(&mSKMListClicked));

    // Multiplayer menu controls
    sheets[multiplayerMenu]->getChild(MPM_BUTTON_SERVER)->subscribeEvent(
        CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&mMPMServerButtonPressed));

    sheets[multiplayerMenu]->getChild(MPM_BUTTON_CLIENT)->subscribeEvent(
        CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&mMPMClientButtonPressed));

    sheets[multiplayerMenu]->getChild(MPM_BUTTON_BACK)->subscribeEvent(
        CEGUI::PushButton::EventClicked,
        CEGUI::Event::Subscriber(&mMPMBackButtonPressed));

    sheets[multiplayerMenu]->getChild(MPM_LIST_LEVELS)->subscribeEvent(
        CEGUI::Listbox::EventMouseDoubleClick,
        CEGUI::Event::Subscriber(&mMPMListClicked));
}

bool Gui::miniMapclicked(const CEGUI::EventArgs& e)
{
    CEGUI::MouseEventArgs& ee = (CEGUI::MouseEventArgs&)e;

    ODFrameListener& frameListener = ODFrameListener::getSingleton();

    Ogre::Vector2 cc = frameListener.getMiniMap()->camera_2dPositionFromClick((int)ee.position.d_x,
        (int)ee.position.d_y);
    frameListener.cm->onMiniMapClick(cc);

    //std::cerr<< xx <<" "<< yy << " " <<std::endl;
    return true;
}

bool Gui::mMNewGameMultiButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm)
        return true;

    mm->requestMenuMultiplayerMode();

    return true;
}

bool Gui::dormitoryButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::dormitory);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "Dormitory");
    return true;
}

bool Gui::treasuryButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::treasury);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "Treasury");
    return true;
}

bool Gui::forgeButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::forge);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "Forge");
    return true;
}

bool Gui::trainingHallButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::trainingHall);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "Training Hall");
    return true;
}

bool Gui::libraryButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::library);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "Library");
    return true;
}

bool Gui::hatcheryButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::hatchery);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "Hatchery");
    return true;
}

bool Gui::cannonButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::nullRoomType);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::cannon);
    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "Cannon");
    return true;
}

bool Gui::confirmExitYesButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm)
        return true;

    mm->requestUnloadToParentGameMode();

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
    return true;
}

bool Gui::confirmExitNoButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::GAME)
        return true;

    static_cast<GameMode*>(mm->getCurrentMode())->popupExit(false);
    return true;
}


// EDITOR

bool Gui::editorGoldButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    static_cast<EditorMode*>(mm->getCurrentMode())->mCurrentTileType = Tile::gold;
    return true;
}

bool Gui::editorLavaButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    static_cast<EditorMode*>(mm->getCurrentMode())->mCurrentTileType = Tile::lava;
    return true;
}

bool Gui::editorRockButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    static_cast<EditorMode*>(mm->getCurrentMode())->mCurrentTileType = Tile::rock;
    return true;
}

bool Gui::editorWaterButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    static_cast<EditorMode*>(mm->getCurrentMode())->mCurrentTileType = Tile::water;
    return true;
}

bool Gui::editorDirtButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    static_cast<EditorMode*>(mm->getCurrentMode())->mCurrentTileType = Tile::dirt;
    return true;
}

bool Gui::editorClaimedButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    static_cast<EditorMode*>(mm->getCurrentMode())->mCurrentTileType = Tile::claimed;
    return true;
}

// MAIN MENU

bool Gui::mMNewGameButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm)
        return true;

    mm->requestMenuSingleplayerMode();

    return true;
}

bool Gui::mMMapEditorButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm)
        return true;

    mm->requestEditorMode();
    return true;
}

bool Gui::mMLoadButtonPressed(const CEGUI::EventArgs& e)
{
    return true;
}

bool Gui::mMOptionsButtonPressed(const CEGUI::EventArgs& e)
{
    return true;
}

bool Gui::mMQuitButtonPressed(const CEGUI::EventArgs& e)
{
    ODFrameListener::getSingletonPtr()->requestExit();
    return true;
}

bool Gui::mSKMBackButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm)
        return true;
    mm->requestUnloadToParentGameMode();

    return true;
}

bool Gui::mSKMListClicked(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::MENU_SKIRMISH)
        return true;

    static_cast<MenuModeSkirmish*>(mm->getCurrentMode())->listLevelsClicked();

    return true;
}

bool Gui::mSKMLoadButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::MENU_SKIRMISH)
        return true;

    static_cast<MenuModeSkirmish*>(mm->getCurrentMode())->launchSelectedButtonPressed();

    return true;
}

bool Gui::mMPMBackButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm)
        return true;
    mm->requestUnloadToParentGameMode();

    return true;
}

bool Gui::mMPMListClicked(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::MENU_MULTIPLAYER)
        return true;

    static_cast<MenuModeMultiplayer*>(mm->getCurrentMode())->listLevelsClicked();

    return true;
}

bool Gui::mMPMServerButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::MENU_MULTIPLAYER)
        return true;

    static_cast<MenuModeMultiplayer*>(mm->getCurrentMode())->serverButtonPressed();

    return true;
}

bool Gui::mMPMClientButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::MENU_MULTIPLAYER)
        return true;

    static_cast<MenuModeMultiplayer*>(mm->getCurrentMode())->clientButtonPressed();

    return true;
}

/* These constants are used to access the GUI element
 * NOTE: when add/remove/rename a GUI element, don't forget to change it here
 */
//TODO: Probably these should be read from a file? Script file?

const std::string Gui::DISPLAY_GOLD = "HorizontalPipe/GoldDisplay";
const std::string Gui::DISPLAY_MANA = "HorizontalPipe/ManaDisplay";
const std::string Gui::DISPLAY_TERRITORY = "HorizontalPipe/TerritoryDisplay";
const std::string Gui::MINIMAP = "MiniMap";
const std::string Gui::MESSAGE_WINDOW = "MessagesDisplayWindow";
const std::string Gui::MAIN_TABCONTROL = "MainTabControl";
const std::string Gui::TAB_ROOMS = "MainTabControl/Rooms";
const std::string Gui::BUTTON_DORMITORY = "MainTabControl/Rooms/DormitoryButton";
const std::string Gui::BUTTON_FORGE = "MainTabControl/Rooms/ForgeButton";
const std::string Gui::BUTTON_TRAININGHALL = "MainTabControl/Rooms/TrainingHallButton";
const std::string Gui::BUTTON_LIBRARY = "MainTabControl/Rooms/LibraryButton";
const std::string Gui::BUTTON_HATCHERY = "MainTabControl/Rooms/HatcheryButton";
const std::string Gui::BUTTON_TREASURY = "MainTabControl/Rooms/TreasuryButton";
const std::string Gui::TAB_TRAPS = "MainTabControl/Traps";
const std::string Gui::BUTTON_CANNON = "MainTabControl/Traps/CannonButton";
const std::string Gui::TAB_SPELLS = "MainTabControl/Spells";
const std::string Gui::TAB_CREATURES = "MainTabControl/Creatures";
const std::string Gui::TAB_COMBAT = "MainTabControl/Combat";

const std::string Gui::MM_BACKGROUND = "Background";
const std::string Gui::MM_WELCOME_MESSAGE = "WelcomeBanner";
const std::string Gui::MM_BUTTON_START_SKIRMISH = "StartSkirmishButton";
const std::string Gui::MM_BUTTON_START_MULTIPLAYER = "StartMultiplayerButton";
const std::string Gui::MM_BUTTON_MAPEDITOR = "MapEditorButton";
const std::string Gui::MM_BUTTON_QUIT = "QuitButton";
const std::string Gui::EXIT_CONFIRMATION_POPUP = "ConfirmExit";
const std::string Gui::EXIT_CONFIRMATION_POPUP_YES_BUTTON = "ConfirmExit/YesOption";
const std::string Gui::EXIT_CONFIRMATION_POPUP_NO_BUTTON = "ConfirmExit/NoOption";

const std::string Gui::SKM_TEXT_LOADING = "LoadingText";
const std::string Gui::SKM_BUTTON_LAUNCH = "LaunchGameButton";
const std::string Gui::SKM_BUTTON_BACK = "BackButton";
const std::string Gui::SKM_LIST_LEVELS = "LevelSelect";

const std::string Gui::MPM_TEXT_LOADING = "LoadingText";
const std::string Gui::MPM_BUTTON_SERVER = "ServerButton";
const std::string Gui::MPM_BUTTON_CLIENT = "ClientButton";
const std::string Gui::MPM_BUTTON_BACK = "BackButton";
const std::string Gui::MPM_LIST_LEVELS = "LevelSelect";
const std::string Gui::MPM_EDIT_IP = "IpEdit";
const std::string Gui::MPM_EDIT_NICK = "NickEdit";

const std::string Gui::EDITOR = "MainTabControl";
const std::string Gui::EDITOR_LAVA_BUTTON = "MainTabControl/Tiles/LavaButton";
const std::string Gui::EDITOR_GOLD_BUTTON = "MainTabControl/Tiles/GoldButton";
const std::string Gui::EDITOR_DIRT_BUTTON = "MainTabControl/Tiles/DirtButton";
const std::string Gui::EDITOR_WATER_BUTTON = "MainTabControl/Tiles/WaterButton";
const std::string Gui::EDITOR_ROCK_BUTTON = "MainTabControl/Tiles/RockButton";
const std::string Gui::EDITOR_CLAIMED_BUTTON = "MainTabControl/Tiles/ClaimedButton";
const std::string Gui::EDITOR_FULLNESS = "HorizontalPipe/FullnessDisplay";
const std::string Gui::EDITOR_CURSOR_POS = "HorizontalPipe/PositionDisplay";
