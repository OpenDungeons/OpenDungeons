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

#include "ODFrameListener.h"
#include "GameMap.h"
#include "Player.h"
#include "Trap.h"
#include "TextRenderer.h"
#include "ODApplication.h"
#include "ODServer.h"
#include "ODClient.h"
#include "CameraManager.h"
#include "ModeManager.h"
#include "GameMode.h"
#include "EditorMode.h"
#include "MenuModeSkirmish.h"
#include "MenuModeMultiplayerClient.h"
#include "MenuModeMultiplayerServer.h"
#include "MenuModeEditor.h"
#include "LogManager.h"
#include "SoundEffectsManager.h"

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
    sheets[multiplayerClientMenu] = wmgr->loadLayoutFromFile("OpenDungeonsMenuMultiplayerClient.layout");
    sheets[multiplayerServerMenu] = wmgr->loadLayoutFromFile("OpenDungeonsMenuMultiplayerServer.layout");
    sheets[editorModeGui] =  wmgr->loadLayoutFromFile("OpenDungeonsEditorModeMenu.layout");
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

    sheets[mainMenu]->getChild(MM_BUTTON_START_MULTIPLAYER_CLIENT)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&mMNewGameMultiClientButtonPressed));

    sheets[mainMenu]->getChild(MM_BUTTON_START_MULTIPLAYER_SERVER)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&mMNewGameMultiServerButtonPressed));

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

    sheets[inGameMenu]->getChild(BUTTON_DESTROY_ROOM)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&destroyRoomButtonPressed));

    sheets[inGameMenu]->getChild(BUTTON_TRAP_CANNON)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&cannonButtonPressed));

    sheets[inGameMenu]->getChild(BUTTON_TRAP_SPIKE)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&spikeTrapButtonPressed));

    sheets[inGameMenu]->getChild(BUTTON_DESTROY_TRAP)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&destroyTrapButtonPressed));

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
    sheets[editorModeGui]->getChild(EDITOR_LAVA_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorLavaButtonPressed));

    sheets[editorModeGui]->getChild(EDITOR_GOLD_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorGoldButtonPressed));

    sheets[editorModeGui]->getChild(EDITOR_ROCK_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorRockButtonPressed));

    sheets[editorModeGui]->getChild(EDITOR_WATER_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorWaterButtonPressed));

    sheets[editorModeGui]->getChild(EDITOR_DIRT_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorDirtButtonPressed));

    sheets[editorModeGui]->getChild(EDITOR_CLAIMED_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorClaimedButtonPressed));
    // Game Mode controls
    sheets[editorModeGui]->getChild(BUTTON_DORMITORY)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&dormitoryButtonPressed));

    sheets[editorModeGui]->getChild(BUTTON_TREASURY)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&treasuryButtonPressed));

    sheets[editorModeGui]->getChild(BUTTON_DESTROY_ROOM)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&destroyRoomButtonPressed));

    sheets[editorModeGui]->getChild(BUTTON_FORGE)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&forgeButtonPressed));

    sheets[editorModeGui]->getChild(BUTTON_TRAININGHALL)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&trainingHallButtonPressed));

    sheets[editorModeGui]->getChild(BUTTON_LIBRARY)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&libraryButtonPressed));

    sheets[editorModeGui]->getChild(BUTTON_HATCHERY)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&hatcheryButtonPressed));

    sheets[editorModeGui]->getChild(BUTTON_TRAP_CANNON)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&cannonButtonPressed));

    sheets[editorModeGui]->getChild(BUTTON_TRAP_SPIKE)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&spikeTrapButtonPressed));

    sheets[editorModeGui]->getChild(BUTTON_DESTROY_TRAP)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&destroyTrapButtonPressed));

    sheets[editorModeGui]->getChild(MINIMAP)->subscribeEvent(
            CEGUI:: Window::EventMouseClick,
            CEGUI::Event::Subscriber(&miniMapclicked));

    // Skirmish level select menu controls
    sheets[skirmishMenu]->getChild(SKM_BUTTON_LAUNCH)->subscribeEvent(
        CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&mSKMLoadButtonPressed));

    sheets[skirmishMenu]->getChild(SKM_BUTTON_BACK)->subscribeEvent(
        CEGUI::PushButton::EventClicked,
        CEGUI::Event::Subscriber(&mSKMBackButtonPressed));

    sheets[skirmishMenu]->getChild(SKM_LIST_LEVELS)->subscribeEvent(
        CEGUI::Listbox::EventMouseClick,
        CEGUI::Event::Subscriber(&mSKMListClicked));

    sheets[skirmishMenu]->getChild(SKM_LIST_LEVELS)->subscribeEvent(
        CEGUI::Listbox::EventMouseDoubleClick,
        CEGUI::Event::Subscriber(&mSKMListDoubleClicked));

    // Multiplayer menu controls
    // Server part
    sheets[multiplayerServerMenu]->getChild(MPM_BUTTON_SERVER)->subscribeEvent(
        CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&mMPMServerButtonPressed));

    sheets[multiplayerServerMenu]->getChild(MPM_LIST_LEVELS)->subscribeEvent(
        CEGUI::Listbox::EventMouseClick,
        CEGUI::Event::Subscriber(&mMPMListClicked));

    sheets[multiplayerServerMenu]->getChild(MPM_LIST_LEVELS)->subscribeEvent(
        CEGUI::Listbox::EventMouseDoubleClick,
        CEGUI::Event::Subscriber(&mMPMListDoubleClicked));

    sheets[multiplayerServerMenu]->getChild(MPM_BUTTON_BACK)->subscribeEvent(
        CEGUI::PushButton::EventClicked,
        CEGUI::Event::Subscriber(&mMPMBackButtonPressed));

    sheets[multiplayerServerMenu]->getChild("LevelWindowFrame/__auto_closebutton__")->subscribeEvent(
        CEGUI::PushButton::EventClicked,
        CEGUI::Event::Subscriber(&mMPMBackButtonPressed));

    // Client part
    sheets[multiplayerClientMenu]->getChild(MPM_BUTTON_CLIENT)->subscribeEvent(
        CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&mMPMClientButtonPressed));

    sheets[multiplayerClientMenu]->getChild(MPM_BUTTON_BACK)->subscribeEvent(
        CEGUI::PushButton::EventClicked,
        CEGUI::Event::Subscriber(&mMPMBackButtonPressed));

    sheets[multiplayerClientMenu]->getChild("LevelWindowFrame/__auto_closebutton__")->subscribeEvent(
        CEGUI::PushButton::EventClicked,
        CEGUI::Event::Subscriber(&mMPMBackButtonPressed));

    // Editor level select menu controls
    sheets[editorMenu]->getChild(EDM_BUTTON_LAUNCH)->subscribeEvent(
        CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&mEDMLoadButtonPressed));

    sheets[editorMenu]->getChild(EDM_BUTTON_BACK)->subscribeEvent(
        CEGUI::PushButton::EventClicked,
        CEGUI::Event::Subscriber(&mEDMBackButtonPressed));

    sheets[editorMenu]->getChild(EDM_LIST_LEVELS)->subscribeEvent(
        CEGUI::Listbox::EventMouseClick,
        CEGUI::Event::Subscriber(&mEDMListClicked));

    sheets[editorMenu]->getChild(EDM_LIST_LEVELS)->subscribeEvent(
        CEGUI::Listbox::EventMouseDoubleClick,
        CEGUI::Event::Subscriber(&mEDMListDoubleClicked));

    // Set the game version
    sheets[mainMenu]->getChild("VersionText")->setText(ODApplication::VERSION);
    sheets[skirmishMenu]->getChild("VersionText")->setText(ODApplication::VERSION);
    sheets[multiplayerServerMenu]->getChild("VersionText")->setText(ODApplication::VERSION);
    sheets[multiplayerClientMenu]->getChild("VersionText")->setText(ODApplication::VERSION);
    sheets[editorMenu]->getChild("VersionText")->setText(ODApplication::VERSION);
}

bool Gui::miniMapclicked(const CEGUI::EventArgs& e)
{
    CEGUI::MouseEventArgs& ee = (CEGUI::MouseEventArgs&)e;

    ODFrameListener& frameListener = ODFrameListener::getSingleton();

    frameListener.onMiniMapClick(static_cast<int>(ee.position.d_x),
        static_cast<int>(ee.position.d_y));
    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);

    //std::cerr<< xx <<" "<< yy << " " <<std::endl;
    return true;
}

bool Gui::mMNewGameMultiClientButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm)
        return true;

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    mm->requestMenuMultiplayerClientMode();
    return true;
}

bool Gui::mMNewGameMultiServerButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm)
        return true;

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    mm->requestMenuMultiplayerServerMode();
    return true;
}

bool Gui::dormitoryButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::dormitory);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildRoom);
    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    return true;
}

bool Gui::treasuryButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::treasury);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildRoom);
    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    return true;
}

bool Gui::destroyRoomButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::nullRoomType);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::destroyRoom);
    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    return true;
}

bool Gui::forgeButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::forge);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildRoom);
    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    return true;
}

bool Gui::trainingHallButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::trainingHall);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildRoom);
    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    return true;
}

bool Gui::libraryButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::library);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildRoom);
    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    return true;
}

bool Gui::hatcheryButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::hatchery);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildRoom);
    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    return true;
}

bool Gui::cannonButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::nullRoomType);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::cannon);
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildTrap);
    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    return true;
}

bool Gui::spikeTrapButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::nullRoomType);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::spike);
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildTrap);
    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    return true;
}

bool Gui::destroyTrapButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::nullRoomType);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::destroyTrap);
    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    return true;
}

bool Gui::confirmExitYesButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm)
        return true;

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    mm->requestUnloadToParentMode();
    return true;
}

bool Gui::confirmExitNoButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::GAME)
        return true;

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    static_cast<GameMode*>(mm->getCurrentMode())->popupExit(false);
    return true;
}


// EDITOR

bool Gui::editorGoldButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::changeTile);
    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    static_cast<EditorMode*>(mm->getCurrentMode())->mCurrentTileType = Tile::gold;
    return true;
}

bool Gui::editorLavaButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::changeTile);
    static_cast<EditorMode*>(mm->getCurrentMode())->mCurrentTileType = Tile::lava;
    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    return true;
}

bool Gui::editorRockButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::changeTile);
    static_cast<EditorMode*>(mm->getCurrentMode())->mCurrentTileType = Tile::rock;
    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    return true;
}

bool Gui::editorWaterButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::changeTile);
    static_cast<EditorMode*>(mm->getCurrentMode())->mCurrentTileType = Tile::water;
    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    return true;
}

bool Gui::editorDirtButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::changeTile);
    static_cast<EditorMode*>(mm->getCurrentMode())->mCurrentTileType = Tile::dirt;
    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    return true;
}

bool Gui::editorClaimedButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::changeTile);
    static_cast<EditorMode*>(mm->getCurrentMode())->mCurrentTileType = Tile::claimed;
    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    return true;
}

// MAIN MENU

bool Gui::mMNewGameButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm)
        return true;

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    mm->requestMenuSingleplayerMode();
    return true;
}

bool Gui::mMMapEditorButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm)
        return true;

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    mm->requestMenuEditorMode();
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

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    mm->requestUnloadToParentMode();
    return true;
}

bool Gui::mSKMListClicked(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::MENU_SKIRMISH)
        return true;

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    static_cast<MenuModeSkirmish*>(mm->getCurrentMode())->listLevelsClicked();
    return true;
}

bool Gui::mSKMListDoubleClicked(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::MENU_SKIRMISH)
        return true;

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    static_cast<MenuModeSkirmish*>(mm->getCurrentMode())->listLevelsDoubleClicked();
    return true;
}

bool Gui::mSKMLoadButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::MENU_SKIRMISH)
        return true;

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    static_cast<MenuModeSkirmish*>(mm->getCurrentMode())->launchSelectedButtonPressed();
    return true;
}

bool Gui::mEDMBackButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm)
        return true;

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    mm->requestUnloadToParentMode();
    return true;
}

bool Gui::mEDMListClicked(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::MENU_EDITOR)
        return true;

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    static_cast<MenuModeEditor*>(mm->getCurrentMode())->listLevelsClicked();
    return true;
}

bool Gui::mEDMListDoubleClicked(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::MENU_EDITOR)
        return true;

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    static_cast<MenuModeEditor*>(mm->getCurrentMode())->listLevelsDoubleClicked();
    return true;
}

bool Gui::mEDMLoadButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::MENU_EDITOR)
        return true;

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    static_cast<MenuModeEditor*>(mm->getCurrentMode())->launchSelectedButtonPressed();
    return true;
}

bool Gui::mMPMBackButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm)
        return true;

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    mm->requestUnloadToParentMode();
    return true;
}

bool Gui::mMPMListClicked(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::MENU_MULTIPLAYER_SERVER)
        return true;

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    static_cast<MenuModeMultiplayerServer*>(mm->getCurrentMode())->listLevelsClicked();
    return true;
}

bool Gui::mMPMListDoubleClicked(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::MENU_MULTIPLAYER_SERVER)
        return true;

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    static_cast<MenuModeMultiplayerServer*>(mm->getCurrentMode())->listLevelsDoubleClicked();
    return true;
}

bool Gui::mMPMServerButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::MENU_MULTIPLAYER_SERVER)
        return true;

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    static_cast<MenuModeMultiplayerServer*>(mm->getCurrentMode())->serverButtonPressed();
    return true;
}

bool Gui::mMPMClientButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::MENU_MULTIPLAYER_CLIENT)
        return true;

    SoundEffectsManager::getSingleton().playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    static_cast<MenuModeMultiplayerClient*>(mm->getCurrentMode())->clientButtonPressed();
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
const std::string Gui::BUTTON_DESTROY_ROOM = "MainTabControl/Rooms/DestroyRoomButton";
const std::string Gui::TAB_TRAPS = "MainTabControl/Traps";
const std::string Gui::BUTTON_TRAP_CANNON = "MainTabControl/Traps/CannonButton";
const std::string Gui::BUTTON_TRAP_SPIKE = "MainTabControl/Traps/SpikeTrapButton";
const std::string Gui::BUTTON_DESTROY_TRAP = "MainTabControl/Traps/DestroyTrapButton";
const std::string Gui::TAB_SPELLS = "MainTabControl/Spells";
const std::string Gui::TAB_CREATURES = "MainTabControl/Creatures";
const std::string Gui::TAB_COMBAT = "MainTabControl/Combat";

const std::string Gui::MM_BACKGROUND = "Background";
const std::string Gui::MM_WELCOME_MESSAGE = "WelcomeBanner";
const std::string Gui::MM_BUTTON_START_SKIRMISH = "StartSkirmishButton";
const std::string Gui::MM_BUTTON_START_MULTIPLAYER_CLIENT = "StartMultiplayerClientButton";
const std::string Gui::MM_BUTTON_START_MULTIPLAYER_SERVER = "StartMultiplayerServerButton";
const std::string Gui::MM_BUTTON_MAPEDITOR = "MapEditorButton";
const std::string Gui::MM_BUTTON_QUIT = "QuitButton";
const std::string Gui::EXIT_CONFIRMATION_POPUP = "ConfirmExit";
const std::string Gui::EXIT_CONFIRMATION_POPUP_YES_BUTTON = "ConfirmExit/YesOption";
const std::string Gui::EXIT_CONFIRMATION_POPUP_NO_BUTTON = "ConfirmExit/NoOption";

const std::string Gui::SKM_TEXT_LOADING = "LoadingText";
const std::string Gui::SKM_BUTTON_LAUNCH = "LevelWindowFrame/LaunchGameButton";
const std::string Gui::SKM_BUTTON_BACK = "LevelWindowFrame/BackButton";
const std::string Gui::SKM_LIST_LEVELS = "LevelWindowFrame/LevelSelect";

const std::string Gui::MPM_TEXT_LOADING = "LoadingText";
const std::string Gui::MPM_BUTTON_SERVER = "LevelWindowFrame/ServerButton";
const std::string Gui::MPM_BUTTON_CLIENT = "LevelWindowFrame/ClientButton";
const std::string Gui::MPM_BUTTON_BACK = "LevelWindowFrame/BackButton";
const std::string Gui::MPM_LIST_LEVELS = "LevelWindowFrame/LevelSelect";
const std::string Gui::MPM_EDIT_IP = "LevelWindowFrame/IpEdit";
const std::string Gui::MPM_EDIT_NICK = "LevelWindowFrame/NickEdit";

const std::string Gui::EDM_TEXT_LOADING = "LoadingText";
const std::string Gui::EDM_BUTTON_LAUNCH = "LevelWindowFrame/LaunchGameButton";
const std::string Gui::EDM_BUTTON_BACK = "LevelWindowFrame/BackButton";
const std::string Gui::EDM_LIST_LEVELS = "LevelWindowFrame/LevelSelect";

const std::string Gui::EDITOR = "MainTabControl";
const std::string Gui::EDITOR_LAVA_BUTTON = "MainTabControl/Tiles/LavaButton";
const std::string Gui::EDITOR_GOLD_BUTTON = "MainTabControl/Tiles/GoldButton";
const std::string Gui::EDITOR_DIRT_BUTTON = "MainTabControl/Tiles/DirtButton";
const std::string Gui::EDITOR_WATER_BUTTON = "MainTabControl/Tiles/WaterButton";
const std::string Gui::EDITOR_ROCK_BUTTON = "MainTabControl/Tiles/RockButton";
const std::string Gui::EDITOR_CLAIMED_BUTTON = "MainTabControl/Tiles/ClaimedButton";
const std::string Gui::EDITOR_FULLNESS = "HorizontalPipe/FullnessDisplay";
const std::string Gui::EDITOR_CURSOR_POS = "HorizontalPipe/PositionDisplay";
const std::string Gui::EDITOR_SEAT_ID = "HorizontalPipe/SeatIdDisplay";
