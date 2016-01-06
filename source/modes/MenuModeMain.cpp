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

    connectModeChangeEvent(Gui::MM_BUTTON_MAPEDITOR, AbstractModeManager::ModeType::MENU_EDITOR_LOAD);
    connectModeChangeEvent(Gui::MM_BUTTON_START_REPLAY, AbstractModeManager::ModeType::MENU_REPLAY);

    addEventConnection(
        rootWin->getChild(Gui::MM_BUTTON_QUIT)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeMain::quitButtonPressed, this)
        )
    );

    addEventConnection(
        rootWin->getChild("SettingsButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeMain::toggleSettings, this)
        )
    );

    // Skirmish & sub-menu events
    addEventConnection(
        rootWin->getChild(Gui::MM_BUTTON_START_SKIRMISH)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeMain::toggleSkirmishSubMenu, this)
        )
    );
    CEGUI::Window* skirmishWin = rootWin->getChild("SkirmishSubMenuWindow");
    OD_ASSERT_TRUE(skirmishWin != nullptr);
    connectModeChangeEvent(skirmishWin->getChild("StartSkirmishButton"),
                           AbstractModeManager::ModeType::MENU_SKIRMISH);
    connectModeChangeEvent(skirmishWin->getChild("LoadSkirmishButton"),
                           AbstractModeManager::ModeType::MENU_LOAD_SAVEDGAME);

    // Multiplayer & sub-menu events
    addEventConnection(
        rootWin->getChild("MultiplayerModeButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeMain::toggleMultiplayerSubMenu, this)
        )
    );
    CEGUI::Window* multiplayerWin = rootWin->getChild("MultiplayerSubMenuWindow");
    OD_ASSERT_TRUE(multiplayerWin != nullptr);
    connectModeChangeEvent(multiplayerWin->getChild("MasterServerJoinButton"),
                           AbstractModeManager::ModeType::MENU_MASTERSERVER_JOIN);
    connectModeChangeEvent(multiplayerWin->getChild("MasterServerHostButton"),
                           AbstractModeManager::ModeType::MENU_MASTERSERVER_HOST);
    connectModeChangeEvent(multiplayerWin->getChild("MultiplayerServerJoinButton"),
                           AbstractModeManager::ModeType::MENU_MULTIPLAYER_CLIENT);
    connectModeChangeEvent(multiplayerWin->getChild("MultiplayerServerHostButton"),
                           AbstractModeManager::ModeType::MENU_MULTIPLAYER_SERVER);
}

void MenuModeMain::activate()
{
    // Loads the corresponding Gui sheet.
    getModeManager().getGui().loadGuiSheet(Gui::mainMenu);
    CEGUI::Window* window = getModeManager().getGui().getGuiSheet(Gui::mainMenu);
    OD_ASSERT_TRUE(window != nullptr);

    window->getChild("SkirmishSubMenuWindow")->hide();
    window->getChild("MultiplayerSubMenuWindow")->hide();

    giveFocus();

    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "");

    // Play the main menu music
    MusicPlayer::getSingleton().play(ConfigManager::getSingleton().getMainMenuMusic());

    GameMap* gameMap = ODFrameListener::getSingletonPtr()->getClientGameMap();
    gameMap->clearAll();
    gameMap->setGamePaused(true);

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
    CEGUI::Window* window = mainWin->getChild("SkirmishSubMenuWindow");
    OD_ASSERT_TRUE(window);
    window->setVisible(!window->isVisible());
    window = mainWin->getChild("MultiplayerSubMenuWindow");
    OD_ASSERT_TRUE(window);
    window->hide();
    return true;
}

bool MenuModeMain::toggleMultiplayerSubMenu(const CEGUI::EventArgs&)
{
    CEGUI::Window* mainWin = getModeManager().getGui().getGuiSheet(Gui::mainMenu);
    OD_ASSERT_TRUE(mainWin);
    CEGUI::Window* window = mainWin->getChild("MultiplayerSubMenuWindow");
    OD_ASSERT_TRUE(window);
    window->setVisible(!window->isVisible());
    window = mainWin->getChild("SkirmishSubMenuWindow");
    OD_ASSERT_TRUE(window);
    window->hide();
    return true;
}
