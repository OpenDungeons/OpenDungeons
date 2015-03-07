/*
 * \file   Gui.cpp
 * \date   05 April 2011
 * \author StefanP.MUC
 * \brief  Class Gui containing all the stuff for the GUI, including translation.
 *
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

#include "render/Gui.h"

#include "ODApplication.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "gamemap/GameMap.h"
#include "modes/EditorMode.h"
#include "modes/GameMode.h"
#include "modes/ModeManager.h"
#include "network/ODClient.h"
#include "network/ODServer.h"
#include "render/ODFrameListener.h"
#include "rooms/Room.h"
#include "sound/SoundEffectsManager.h"
#include "spell/Spell.h"
#include "traps/Trap.h"
#include "utils/LogManager.h"

#include <CEGUI/CEGUI.h>
#include <CEGUI/RendererModules/Ogre/Renderer.h>
#include <CEGUI/RendererModules/Ogre/ResourceProvider.h>
#include <CEGUI/RendererModules/Ogre/ImageCodec.h>
#include <CEGUI/SchemeManager.h>
#include <CEGUI/System.h>
#include <CEGUI/WindowManager.h>
#include <CEGUI/widgets/PushButton.h>
#include <CEGUI/Event.h>

Gui::Gui(SoundEffectsManager* soundEffectsManager, const std::string& ceguiLogFileName)
  : mSoundEffectsManager(soundEffectsManager)
{
    CEGUI::OgreRenderer& renderer = CEGUI::OgreRenderer::create();
    CEGUI::OgreResourceProvider& rp = CEGUI::OgreRenderer::createOgreResourceProvider();
    CEGUI::OgreImageCodec& ic = CEGUI::OgreRenderer::createOgreImageCodec();
    CEGUI::System::create(renderer, &rp, static_cast<CEGUI::XMLParser*>(nullptr), &ic, nullptr, "",
                          ceguiLogFileName);

    CEGUI::SchemeManager::getSingleton().createFromFile("OpenDungeonsSkin.scheme");

    // Needed to get the correct offset when using up to CEGUI 0.8.4
    // We're thus using an empty mouse cursor.
    CEGUI::GUIContext& context = CEGUI::System::getSingleton().getDefaultGUIContext();
    context.getMouseCursor().setDefaultImage("OpenDungeonsSkin/MouseArrow");
    context.getMouseCursor().setVisible(true);

    context.setDefaultTooltipType("OD/Tooltip");


    CEGUI::WindowManager* wmgr = CEGUI::WindowManager::getSingletonPtr();

    CEGUI::Window* myHide = wmgr->createWindow("DefaultWindow", "DummyWindow");
    mSheets[hideGui] = myHide;

    mSheets[inGameMenu] = wmgr->loadLayoutFromFile("OpenDungeonsGameModeMenu.layout");
    mSheets[mainMenu] = wmgr->loadLayoutFromFile("OpenDungeonsMainMenu.layout");
    mSheets[skirmishMenu] = wmgr->loadLayoutFromFile("OpenDungeonsMenuSkirmish.layout");
    mSheets[multiplayerClientMenu] = wmgr->loadLayoutFromFile("OpenDungeonsMenuMultiplayerClient.layout");
    mSheets[multiplayerServerMenu] = wmgr->loadLayoutFromFile("OpenDungeonsMenuMultiplayerServer.layout");
    mSheets[editorModeGui] =  wmgr->loadLayoutFromFile("OpenDungeonsEditorModeMenu.layout");
    mSheets[editorMenu] =  wmgr->loadLayoutFromFile("OpenDungeonsEditorMenu.layout");
    mSheets[configureSeats] =  wmgr->loadLayoutFromFile("OpenDungeonsMenuConfigureSeats.layout");
    mSheets[replayMenu] =  wmgr->loadLayoutFromFile("OpenDungeonsMenuReplay.layout");
    mSheets[loadSavedGameMenu] =  wmgr->loadLayoutFromFile("OpenDungeonsMenuLoad.layout");
    mSheets[console] = wmgr->loadLayoutFromFile("OpenDungeonsConsole.layout");

    assignEventHandlers();

    //Add sound to button clicks and selection
    CEGUI::GlobalEventSet& ges = CEGUI::GlobalEventSet::getSingleton();
    ges.subscribeEvent(
        CEGUI::PushButton::EventNamespace + "/" + CEGUI::PushButton::EventClicked,
        CEGUI::Event::Subscriber(&Gui::playButtonClickSound, this));
    ges.subscribeEvent(
        CEGUI::Listbox::EventNamespace + "/" + CEGUI::Listbox::EventSelectionChanged,
        CEGUI::Event::Subscriber(&Gui::playButtonClickSound, this));
}

Gui::~Gui()
{
    //This also calls CEGUI::System::destroy();
    CEGUI::OgreRenderer::destroySystem();
}

CEGUI::MouseButton Gui::convertButton(OIS::MouseButtonID buttonID)
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
    CEGUI::System::getSingletonPtr()->getDefaultGUIContext().setRootWindow(mSheets[newSheet]);
    // This shouldn't be needed, but the gui seems to not allways change when using hideGui without it.
    CEGUI::System::getSingletonPtr()->getDefaultGUIContext().markAsDirty();
}

CEGUI::Window* Gui::getGuiSheet(const guiSheet& sheet)
{
    if (mSheets.find(sheet) != mSheets.end())
    {
        return mSheets[sheet];
    }
    return nullptr;
}

void Gui::assignEventHandlers()
{
    CEGUI::Window* rootWindow = mSheets[inGameMenu];

    if (rootWindow == nullptr)
    {
        LogManager::getSingleton().logMessage("Gui: No root window!");
        return;
    }

    // Game Mode controls
    mSheets[inGameMenu]->getChild(BUTTON_DORMITORY)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&dormitoryButtonPressed));

    mSheets[inGameMenu]->getChild(BUTTON_TREASURY)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&treasuryButtonPressed));

    mSheets[inGameMenu]->getChild(BUTTON_FORGE)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&forgeButtonPressed));

    mSheets[inGameMenu]->getChild(BUTTON_TRAININGHALL)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&trainingHallButtonPressed));

    mSheets[inGameMenu]->getChild(BUTTON_LIBRARY)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&libraryButtonPressed));

    mSheets[inGameMenu]->getChild(BUTTON_HATCHERY)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&hatcheryButtonPressed));

    mSheets[inGameMenu]->getChild(BUTTON_CRYPT)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&cryptButtonPressed));

    mSheets[inGameMenu]->getChild(BUTTON_DESTROY_ROOM)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&destroyRoomButtonPressed));

    mSheets[inGameMenu]->getChild(BUTTON_TRAP_CANNON)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&cannonButtonPressed));

    mSheets[inGameMenu]->getChild(BUTTON_TRAP_SPIKE)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&spikeTrapButtonPressed));

    mSheets[inGameMenu]->getChild(BUTTON_TRAP_BOULDER)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&boulderTrapButtonPressed));

    mSheets[inGameMenu]->getChild(BUTTON_DESTROY_TRAP)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&destroyTrapButtonPressed));

    mSheets[inGameMenu]->getChild(BUTTON_SPELL_SUMMON_WORKER)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&spellSummonWorkerPressed));

    mSheets[inGameMenu]->getChild(BUTTON_SPELL_CALLTOWAR)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&spellCallToWarPressed));

    mSheets[inGameMenu]->getChild(BUTTON_CREATURE_WORKER)->subscribeEvent(
            CEGUI::Window::EventMouseClick,
            CEGUI::Event::Subscriber(&workerCreatureButtonPressed));

    mSheets[inGameMenu]->getChild(BUTTON_CREATURE_FIGHTER)->subscribeEvent(
            CEGUI::Window::EventMouseClick,
            CEGUI::Event::Subscriber(&fighterCreatureButtonPressed));

    mSheets[inGameMenu]->getChild(EXIT_CONFIRMATION_POPUP_YES_BUTTON)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&confirmExitYesButtonPressed));

    mSheets[inGameMenu]->getChild(EXIT_CONFIRMATION_POPUP_NO_BUTTON)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&confirmExitNoButtonPressed));

    mSheets[inGameMenu]->getChild("ConfirmExit/__auto_closebutton__")->subscribeEvent(
        CEGUI::PushButton::EventClicked,
        CEGUI::Event::Subscriber(&confirmExitNoButtonPressed));

    mSheets[inGameMenu]->getChild("HelpButton")->subscribeEvent(
        CEGUI::PushButton::EventClicked,
        CEGUI::Event::Subscriber(&helpButtonPressed));

    mSheets[inGameMenu]->getChild("ObjectivesButton")->subscribeEvent(
        CEGUI::PushButton::EventClicked,
        CEGUI::Event::Subscriber(&objectivesButtonPressed));

    mSheets[inGameMenu]->getChild("OptionsButton")->subscribeEvent(
        CEGUI::PushButton::EventClicked,
        CEGUI::Event::Subscriber(&optionsButtonPressed));

    // Search for the autoclose button and make it work
    mSheets[inGameMenu]->getChild("ObjectivesWindow/__auto_closebutton__")->subscribeEvent(
        CEGUI::PushButton::EventClicked,
        CEGUI::Event::Subscriber(&hideObjectivesWindow));

    // Search for the autoclose button and make it work
    mSheets[inGameMenu]->getChild("SettingsWindow/__auto_closebutton__")->subscribeEvent(
        CEGUI::PushButton::EventClicked,
        CEGUI::Event::Subscriber(&cancelSettings));

    mSheets[inGameMenu]->getChild("SettingsWindow/CancelButton")->subscribeEvent(
        CEGUI::PushButton::EventClicked,
        CEGUI::Event::Subscriber(&cancelSettings));

    // Editor Mode controls
    mSheets[editorModeGui]->getChild("OptionsButton")->subscribeEvent(
        CEGUI::PushButton::EventClicked,
        CEGUI::Event::Subscriber(&editorOptionsButtonPressed));

    mSheets[editorModeGui]->getChild(EDITOR_LAVA_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorLavaButtonPressed));

    mSheets[editorModeGui]->getChild(EDITOR_GOLD_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorGoldButtonPressed));

    mSheets[editorModeGui]->getChild(EDITOR_ROCK_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorRockButtonPressed));

    mSheets[editorModeGui]->getChild(EDITOR_WATER_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorWaterButtonPressed));

    mSheets[editorModeGui]->getChild(EDITOR_DIRT_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorDirtButtonPressed));

    mSheets[editorModeGui]->getChild(EDITOR_CLAIMED_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorClaimedButtonPressed));

    mSheets[editorModeGui]->getChild(EDITOR_MAPLIGHT_BUTTON)->subscribeEvent(
        CEGUI:: Window::EventMouseClick,
        CEGUI::Event::Subscriber(&editorMapLightButtonPressed));

    // Game Mode controls
    mSheets[editorModeGui]->getChild(BUTTON_DORMITORY)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&dormitoryButtonPressed));

    mSheets[editorModeGui]->getChild(BUTTON_TREASURY)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&treasuryButtonPressed));

    mSheets[editorModeGui]->getChild(BUTTON_DESTROY_ROOM)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&destroyRoomButtonPressed));

    mSheets[editorModeGui]->getChild(BUTTON_FORGE)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&forgeButtonPressed));

    mSheets[editorModeGui]->getChild(BUTTON_TRAININGHALL)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&trainingHallButtonPressed));

    mSheets[editorModeGui]->getChild(BUTTON_LIBRARY)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&libraryButtonPressed));

    mSheets[editorModeGui]->getChild(BUTTON_HATCHERY)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&hatcheryButtonPressed));

    mSheets[editorModeGui]->getChild(BUTTON_CRYPT)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&cryptButtonPressed));

    mSheets[editorModeGui]->getChild(BUTTON_TEMPLE)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&templeButtonPressed));

    mSheets[editorModeGui]->getChild(BUTTON_PORTAL)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&portalButtonPressed));

    mSheets[editorModeGui]->getChild(BUTTON_TRAP_CANNON)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&cannonButtonPressed));

    mSheets[editorModeGui]->getChild(BUTTON_TRAP_SPIKE)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&spikeTrapButtonPressed));

    mSheets[editorModeGui]->getChild(BUTTON_TRAP_BOULDER)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&boulderTrapButtonPressed));

    mSheets[editorModeGui]->getChild(BUTTON_DESTROY_TRAP)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&destroyTrapButtonPressed));

    mSheets[editorModeGui]->getChild(BUTTON_CREATURE_WORKER)->subscribeEvent(
            CEGUI::Window::EventMouseClick,
            CEGUI::Event::Subscriber(&workerCreatureButtonPressed));

    mSheets[editorModeGui]->getChild(BUTTON_CREATURE_FIGHTER)->subscribeEvent(
            CEGUI::Window::EventMouseClick,
            CEGUI::Event::Subscriber(&fighterCreatureButtonPressed));

    // Set the game version
    mSheets[mainMenu]->getChild("VersionText")->setText(ODApplication::VERSION);
    mSheets[skirmishMenu]->getChild("VersionText")->setText(ODApplication::VERSION);
    mSheets[multiplayerServerMenu]->getChild("VersionText")->setText(ODApplication::VERSION);
    mSheets[multiplayerClientMenu]->getChild("VersionText")->setText(ODApplication::VERSION);
    mSheets[editorMenu]->getChild("VersionText")->setText(ODApplication::VERSION);
    mSheets[replayMenu]->getChild("VersionText")->setText(ODApplication::VERSION);
    mSheets[loadSavedGameMenu]->getChild("VersionText")->setText(ODApplication::VERSION);
}

bool Gui::playButtonClickSound(const CEGUI::EventArgs&)
{
    mSoundEffectsManager->playInterfaceSound(SoundEffectsManager::BUTTONCLICK);
    return true;
}

bool Gui::dormitoryButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildRoom);
    gameMap->getLocalPlayer()->setNewRoomType(RoomType::dormitory);
    return true;
}

bool Gui::treasuryButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildRoom);
    gameMap->getLocalPlayer()->setNewRoomType(RoomType::treasury);
    return true;
}

bool Gui::destroyRoomButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::destroyRoom);
    return true;
}

bool Gui::forgeButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildRoom);
    gameMap->getLocalPlayer()->setNewRoomType(RoomType::forge);
    return true;
}

bool Gui::trainingHallButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildRoom);
    gameMap->getLocalPlayer()->setNewRoomType(RoomType::trainingHall);
    return true;
}

bool Gui::libraryButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildRoom);
    gameMap->getLocalPlayer()->setNewRoomType(RoomType::library);
    return true;
}

bool Gui::hatcheryButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildRoom);
    gameMap->getLocalPlayer()->setNewRoomType(RoomType::hatchery);
    return true;
}

bool Gui::cryptButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildRoom);
    gameMap->getLocalPlayer()->setNewRoomType(RoomType::crypt);
    return true;
}

bool Gui::templeButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildRoom);
    gameMap->getLocalPlayer()->setNewRoomType(RoomType::dungeonTemple);
    return true;
}

bool Gui::portalButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildRoom);
    gameMap->getLocalPlayer()->setNewRoomType(RoomType::portal);
    return true;
}

bool Gui::cannonButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildTrap);
    gameMap->getLocalPlayer()->setNewTrapType(TrapType::cannon);
    return true;
}

bool Gui::spikeTrapButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildTrap);
    gameMap->getLocalPlayer()->setNewTrapType(TrapType::spike);
    return true;
}

bool Gui::boulderTrapButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::buildTrap);
    gameMap->getLocalPlayer()->setNewTrapType(TrapType::boulder);
    return true;
}

bool Gui::destroyTrapButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::destroyTrap);
    return true;
}

bool Gui::spellSummonWorkerPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::castSpell);
    gameMap->getLocalPlayer()->setNewSpellType(SpellType::summonWorker);
    return true;
}

bool Gui::spellCallToWarPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::castSpell);
    gameMap->getLocalPlayer()->setNewSpellType(SpellType::callToWar);
    return true;
}

bool Gui::workerCreatureButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if ((mm == nullptr) || (mm->getCurrentMode() == nullptr))
        return true;

    mm->getCurrentMode()->notifyGuiAction(AbstractApplicationMode::GuiAction::ButtonPressedCreatureWorker);
    return true;
}

bool Gui::fighterCreatureButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if ((mm == nullptr) || (mm->getCurrentMode() == nullptr))
        return true;

    mm->getCurrentMode()->notifyGuiAction(AbstractApplicationMode::GuiAction::ButtonPressedCreatureFighter);
    return true;
}

bool Gui::confirmExitYesButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm)
        return true;

    mm->requestUnloadToParentMode();
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

bool Gui::helpButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::GAME)
        return true;

    static_cast<GameMode*>(mm->getCurrentMode())->toggleHelpWindow();
    return true;
}

bool Gui::objectivesButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::GAME)
        return true;

    static_cast<GameMode*>(mm->getCurrentMode())->toggleObjectivesWindow();
    return true;
}

bool Gui::hideObjectivesWindow(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::GAME)
        return true;

    static_cast<GameMode*>(mm->getCurrentMode())->hideObjectivesWindow();
    return true;
}

bool Gui::optionsButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::GAME)
        return true;

    static_cast<GameMode*>(mm->getCurrentMode())->toggleOptionsWindow();
    return true;
}

bool Gui::cancelSettings(const CEGUI::EventArgs& e)
{
    // TODO: restore previous settings...
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::GAME)
        return true;

    static_cast<GameMode*>(mm->getCurrentMode())->hideSettingsWindow();
    return true;
}

// EDITOR

bool Gui::editorOptionsButtonPressed(const CEGUI::EventArgs &e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    static_cast<EditorMode*>(mm->getCurrentMode())->toggleOptionsWindow();
    return true;
}

bool Gui::editorGoldButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::changeTile);
    static_cast<EditorMode*>(mm->getCurrentMode())->mCurrentTileVisual = TileVisual::gold;
    return true;
}

bool Gui::editorLavaButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::changeTile);
    static_cast<EditorMode*>(mm->getCurrentMode())->mCurrentTileVisual = TileVisual::lava;
    return true;
}

bool Gui::editorRockButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::changeTile);
    static_cast<EditorMode*>(mm->getCurrentMode())->mCurrentTileVisual = TileVisual::rock;
    return true;
}

bool Gui::editorWaterButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::changeTile);
    static_cast<EditorMode*>(mm->getCurrentMode())->mCurrentTileVisual = TileVisual::water;
    return true;
}

bool Gui::editorDirtButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::changeTile);
    static_cast<EditorMode*>(mm->getCurrentMode())->mCurrentTileVisual = TileVisual::dirt;
    return true;
}

bool Gui::editorClaimedButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->getLocalPlayer()->setCurrentAction(Player::SelectedAction::changeTile);
    static_cast<EditorMode*>(mm->getCurrentMode())->mCurrentTileVisual = TileVisual::claimed;
    return true;
}

bool Gui::editorMapLightButtonPressed(const CEGUI::EventArgs& e)
{
    ModeManager* mm = ODFrameListener::getSingleton().getModeManager();
    if (!mm || mm->getCurrentModeType() != ModeManager::EDITOR)
        return true;

    mm->getCurrentMode()->notifyGuiAction(AbstractApplicationMode::GuiAction::ButtonPressedMapLight);
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
const std::string Gui::OBJECTIVE_TEXT = "ObjectivesWindow/ObjectivesText";
const std::string Gui::MAIN_TABCONTROL = "MainTabControl";
const std::string Gui::TAB_ROOMS = "MainTabControl/Rooms";
const std::string Gui::BUTTON_DORMITORY = "MainTabControl/Rooms/DormitoryButton";
const std::string Gui::BUTTON_FORGE = "MainTabControl/Rooms/ForgeButton";
const std::string Gui::BUTTON_TRAININGHALL = "MainTabControl/Rooms/TrainingHallButton";
const std::string Gui::BUTTON_LIBRARY = "MainTabControl/Rooms/LibraryButton";
const std::string Gui::BUTTON_HATCHERY = "MainTabControl/Rooms/HatcheryButton";
const std::string Gui::BUTTON_TREASURY = "MainTabControl/Rooms/TreasuryButton";
const std::string Gui::BUTTON_CRYPT = "MainTabControl/Rooms/CryptButton";
const std::string Gui::BUTTON_TEMPLE = "MainTabControl/Rooms/TempleButton";
const std::string Gui::BUTTON_PORTAL = "MainTabControl/Rooms/PortalButton";
const std::string Gui::BUTTON_DESTROY_ROOM = "MainTabControl/Rooms/DestroyRoomButton";
const std::string Gui::TAB_TRAPS = "MainTabControl/Traps";
const std::string Gui::BUTTON_TRAP_CANNON = "MainTabControl/Traps/CannonButton";
const std::string Gui::BUTTON_TRAP_SPIKE = "MainTabControl/Traps/SpikeTrapButton";
const std::string Gui::BUTTON_TRAP_BOULDER = "MainTabControl/Traps/BoulderTrapButton";
const std::string Gui::BUTTON_DESTROY_TRAP = "MainTabControl/Traps/DestroyTrapButton";
const std::string Gui::TAB_SPELLS = "MainTabControl/Spells";
const std::string Gui::BUTTON_SPELL_SUMMON_WORKER = "MainTabControl/Spells/SummonWorkerButton";
const std::string Gui::BUTTON_SPELL_CALLTOWAR = "MainTabControl/Spells/CallToWarButton";
const std::string Gui::TAB_CREATURES = "MainTabControl/Creatures";
const std::string Gui::BUTTON_CREATURE_WORKER = "MainTabControl/Creatures/WorkerButton";
const std::string Gui::BUTTON_CREATURE_FIGHTER = "MainTabControl/Creatures/FighterButton";
const std::string Gui::TAB_COMBAT = "MainTabControl/Combat";

const std::string Gui::MM_BACKGROUND = "Background";
const std::string Gui::MM_WELCOME_MESSAGE = "WelcomeBanner";
const std::string Gui::MM_BUTTON_START_SKIRMISH = "StartSkirmishButton";
const std::string Gui::MM_BUTTON_START_REPLAY = "StartReplayButton";
const std::string Gui::MM_BUTTON_START_MULTIPLAYER_CLIENT = "StartMultiplayerClientButton";
const std::string Gui::MM_BUTTON_START_MULTIPLAYER_SERVER = "StartMultiplayerServerButton";
const std::string Gui::MM_BUTTON_LOAD_GAME = "LoadGameButton";
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

const std::string Gui::CSM_BUTTON_LAUNCH = "ListPlayers/LaunchGameButton";
const std::string Gui::CSM_BUTTON_BACK = "ListPlayers/BackButton";

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
const std::string Gui::EDITOR_CREATURE_SPAWN = "HorizontalPipe/CreatureSpawnDisplay";
const std::string Gui::EDITOR_MAPLIGHT_BUTTON = "MainTabControl/Lights/MapLightButton";

const std::string Gui::REM_TEXT_LOADING = "LoadingText";
const std::string Gui::REM_BUTTON_LAUNCH = "LevelWindowFrame/LaunchReplayButton";
const std::string Gui::REM_BUTTON_DELETE = "LevelWindowFrame/DeleteReplayButton";
const std::string Gui::REM_BUTTON_BACK = "LevelWindowFrame/BackButton";
const std::string Gui::REM_LIST_REPLAYS = "LevelWindowFrame/ReplaySelect";
