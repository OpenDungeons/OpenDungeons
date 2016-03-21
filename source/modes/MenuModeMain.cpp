/*
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

#include "MenuModeMain.h"

#include "modes/ModeManager.h"

#include "ODApplication.h"
#include "gamemap/GameMap.h"
#include "render/Gui.h"
#include "render/ODFrameListener.h"
#include "render/TextRenderer.h"
#include "sound/MusicPlayer.h"
#include "utils/ConfigManager.h"
#include "utils/LogManager.h"

#include <CEGUI/widgets/PushButton.h>

// Main buttons
const std::string BUTTON_SKIRMISH = "StartSkirmishButton";
const std::string BUTTON_START_REPLAY = "StartReplayButton";
const std::string BUTTON_MAPEDITOR = "MapEditorButton";
const std::string BUTTON_MULTIPLAYER = "MultiplayerModeButton";
const std::string BUTTON_SETTINGS = "SettingsButton";
const std::string BUTTON_QUIT = "QuitButton";

// Sub-menus windows & buttons
const std::string WINDOW_SKIRMISH = "SkirmishSubMenuWindow";
const std::string WINDOW_MULTIPLAYER = "MultiplayerSubMenuWindow";
const std::string WINDOW_EDITOR = "EditorSubMenuWindow";

const std::string BUTTON_START_SKIRMISH = "StartSkirmishButton";
const std::string BUTTON_LOAD_SKIRMISH = "LoadSkirmishButton";
const std::string BUTTON_MASTERSERVER_JOIN = "MasterServerJoinButton";
const std::string BUTTON_MASTERSERVER_HOST = "MasterServerHostButton";
const std::string BUTTON_MULTIPLAYER_JOIN = "MultiplayerServerJoinButton";
const std::string BUTTON_MULTIPLAYER_HOST = "MultiplayerServerHostButton";
const std::string BUTTON_EDITOR_NEW = "EditorNewButton";
const std::string BUTTON_EDITOR_LOAD = "EditorLoadButton";

namespace
{
//! \brief Helper functor to change modes
class ModeChanger
{
public:
    bool operator()(const CEGUI::EventArgs& e)
    {
        mMode->changeModeEvent(mNewMode, e);
        return true;
    }

    MenuModeMain* mMode;
    AbstractModeManager::ModeType mNewMode;
};
} // namespace

MenuModeMain::MenuModeMain(ModeManager *modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU_MAIN),
    mSettings(SettingsWindow(getModeManager().getGui().getGuiSheet(Gui::mainMenu)))
{
    CEGUI::Window* rootWin = getModeManager().getGui().getGuiSheet(Gui::mainMenu);
    OD_ASSERT_TRUE(rootWin != nullptr);

    connectModeChangeEvent(BUTTON_START_REPLAY, AbstractModeManager::ModeType::MENU_REPLAY);

    addEventConnection(
        rootWin->getChild(BUTTON_QUIT)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeMain::quitButtonPressed, this)
        )
    );

    addEventConnection(
        rootWin->getChild(BUTTON_SETTINGS)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeMain::toggleSettings, this)
        )
    );

    // Skirmish & sub-menu events
    addEventConnection(
        rootWin->getChild(BUTTON_SKIRMISH)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeMain::toggleSkirmishSubMenu, this)
        )
    );
    CEGUI::Window* skirmishWin = rootWin->getChild(WINDOW_SKIRMISH);
    OD_ASSERT_TRUE(skirmishWin != nullptr);
    connectModeChangeEvent(skirmishWin->getChild(BUTTON_START_SKIRMISH),
                           AbstractModeManager::ModeType::MENU_SKIRMISH);
    connectModeChangeEvent(skirmishWin->getChild(BUTTON_LOAD_SKIRMISH),
                           AbstractModeManager::ModeType::MENU_LOAD_SAVEDGAME);

    // Multiplayer & sub-menu events
    addEventConnection(
        rootWin->getChild(BUTTON_MULTIPLAYER)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeMain::toggleMultiplayerSubMenu, this)
        )
    );
    CEGUI::Window* multiplayerWin = rootWin->getChild(WINDOW_MULTIPLAYER);
    OD_ASSERT_TRUE(multiplayerWin != nullptr);
    connectModeChangeEvent(multiplayerWin->getChild(BUTTON_MASTERSERVER_JOIN),
                           AbstractModeManager::ModeType::MENU_MASTERSERVER_JOIN);
    connectModeChangeEvent(multiplayerWin->getChild(BUTTON_MASTERSERVER_HOST),
                           AbstractModeManager::ModeType::MENU_MASTERSERVER_HOST);
    connectModeChangeEvent(multiplayerWin->getChild(BUTTON_MULTIPLAYER_JOIN),
                           AbstractModeManager::ModeType::MENU_MULTIPLAYER_CLIENT);
    connectModeChangeEvent(multiplayerWin->getChild(BUTTON_MULTIPLAYER_HOST),
                           AbstractModeManager::ModeType::MENU_MULTIPLAYER_SERVER);

    // Editor & sub-menu events
    addEventConnection(
        rootWin->getChild(BUTTON_MAPEDITOR)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeMain::toggleEditorSubMenu, this)
        )
    );
    CEGUI::Window* editorWin = rootWin->getChild(WINDOW_EDITOR);
    OD_ASSERT_TRUE(editorWin != nullptr);
    connectModeChangeEvent(editorWin->getChild(BUTTON_EDITOR_NEW),
                           AbstractModeManager::ModeType::MENU_EDITOR_NEW);
    connectModeChangeEvent(editorWin->getChild(BUTTON_EDITOR_LOAD),
                           AbstractModeManager::ModeType::MENU_EDITOR_LOAD);
}

void MenuModeMain::activate()
{
    // Loads the corresponding Gui sheet.
    getModeManager().getGui().loadGuiSheet(Gui::mainMenu);
    CEGUI::Window* window = getModeManager().getGui().getGuiSheet(Gui::mainMenu);
    OD_ASSERT_TRUE(window != nullptr);

    window->getChild(WINDOW_SKIRMISH)->hide();
    window->getChild(WINDOW_MULTIPLAYER)->hide();
    window->getChild(WINDOW_EDITOR)->hide();

    giveFocus();

    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "");

    // Play the main menu music
    MusicPlayer::getSingleton().play(ConfigManager::getSingleton().getMainMenuMusic());

    GameMap* gameMap = ODFrameListener::getSingletonPtr()->getClientGameMap();
    gameMap->clearAll();
    gameMap->setGamePaused(true);

    ODFrameListener::getSingleton().stopGameRenderer();
    ODFrameListener::getSingleton().createMainMenuScene();
}

void MenuModeMain::connectModeChangeEvent(const std::string& buttonName, AbstractModeManager::ModeType mode)
{
    CEGUI::Window* window = getModeManager().getGui().getGuiSheet(Gui::mainMenu);
    connectModeChangeEvent(window->getChild(buttonName), mode);
}

void MenuModeMain::connectModeChangeEvent(CEGUI::Window* button, AbstractModeManager::ModeType mode)
{
    OD_ASSERT_TRUE(button != nullptr);
    addEventConnection(
        button->subscribeEvent(
          CEGUI::PushButton::EventClicked,
          CEGUI::Event::Subscriber(ModeChanger{this, mode})
        )
    );
}

bool MenuModeMain::quitButtonPressed(const CEGUI::EventArgs&)
{
    ODFrameListener::getSingletonPtr()->requestExit();
    return true;
}

bool MenuModeMain::toggleSettings(const CEGUI::EventArgs&)
{
    if (mSettings.isVisible())
        mSettings.onCancelSettings();
    else
        mSettings.show();
    return true;
}

bool MenuModeMain::toggleSkirmishSubMenu(const CEGUI::EventArgs&)
{
    CEGUI::Window* mainWin = getModeManager().getGui().getGuiSheet(Gui::mainMenu);
    OD_ASSERT_TRUE(mainWin);
    CEGUI::Window* window = mainWin->getChild(WINDOW_SKIRMISH);
    OD_ASSERT_TRUE(window);
    window->setVisible(!window->isVisible());
    mainWin->getChild(WINDOW_MULTIPLAYER)->hide();
    mainWin->getChild(WINDOW_EDITOR)->hide();
    return true;
}

bool MenuModeMain::toggleMultiplayerSubMenu(const CEGUI::EventArgs&)
{
    CEGUI::Window* mainWin = getModeManager().getGui().getGuiSheet(Gui::mainMenu);
    OD_ASSERT_TRUE(mainWin);
    CEGUI::Window* window = mainWin->getChild(WINDOW_MULTIPLAYER);
    OD_ASSERT_TRUE(window);
    window->setVisible(!window->isVisible());
    mainWin->getChild(WINDOW_SKIRMISH)->hide();
    mainWin->getChild(WINDOW_EDITOR)->hide();
    return true;
}

bool MenuModeMain::toggleEditorSubMenu(const CEGUI::EventArgs&)
{
    CEGUI::Window* mainWin = getModeManager().getGui().getGuiSheet(Gui::mainMenu);
    OD_ASSERT_TRUE(mainWin);
    CEGUI::Window* window = mainWin->getChild(WINDOW_EDITOR);
    OD_ASSERT_TRUE(window);
    window->setVisible(!window->isVisible());
    mainWin->getChild(WINDOW_MULTIPLAYER)->hide();
    mainWin->getChild(WINDOW_SKIRMISH)->hide();
    return true;
}
