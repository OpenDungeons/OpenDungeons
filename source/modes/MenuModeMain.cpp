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
            CEGUI::Event::Subscriber(&MenuModeMain::openSkirmishSubMenu, this)
        )
    );
    CEGUI::Window* skirmishWin = rootWin->getChild("SkirmishSubMenuWindow");
    addEventConnection(
        skirmishWin->getChild("QuitSkirmishMenuButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeMain::closeSkirmishSubMenu, this)
        )
    );
    addEventConnection(
        skirmishWin->getChild("__auto_closebutton__")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeMain::closeSkirmishSubMenu, this)
        )
    );
    addEventConnection(
        skirmishWin->getChild("StartSkirmishButton")->subscribeEvent(
          CEGUI::PushButton::EventClicked,
          CEGUI::Event::Subscriber(&MenuModeMain::startSkirmish, this)
        )
    );
    addEventConnection(
        skirmishWin->getChild("LoadSkirmishButton")->subscribeEvent(
          CEGUI::PushButton::EventClicked,
          CEGUI::Event::Subscriber(&MenuModeMain::loadSkirmish, this)
        )
    );

    // Multiplayer & sub-menu events
    addEventConnection(
        rootWin->getChild("MultiplayerModeButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeMain::openMultiplayerSubMenu, this)
        )
    );
    CEGUI::Window* multiplayerWin = rootWin->getChild("MultiplayerSubMenuWindow");
    addEventConnection(
        multiplayerWin->getChild("QuitMultiplayerMenuButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeMain::closeMultiplayerSubMenu, this)
        )
    );
    addEventConnection(
        multiplayerWin->getChild("__auto_closebutton__")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeMain::closeMultiplayerSubMenu, this)
        )
    );
    addEventConnection(
        multiplayerWin->getChild("MasterServerJoinButton")->subscribeEvent(
          CEGUI::PushButton::EventClicked,
          CEGUI::Event::Subscriber(&MenuModeMain::joinMasterServerGame, this)
        )
    );
    addEventConnection(
        multiplayerWin->getChild("MasterServerHostButton")->subscribeEvent(
          CEGUI::PushButton::EventClicked,
          CEGUI::Event::Subscriber(&MenuModeMain::hostMasterServerGame, this)
        )
    );
    addEventConnection(
        multiplayerWin->getChild("MultiplayerServerJoinButton")->subscribeEvent(
          CEGUI::PushButton::EventClicked,
          CEGUI::Event::Subscriber(&MenuModeMain::joinMultiplayerGame, this)
        )
    );
    addEventConnection(
        multiplayerWin->getChild("MultiplayerServerHostButton")->subscribeEvent(
          CEGUI::PushButton::EventClicked,
          CEGUI::Event::Subscriber(&MenuModeMain::hostMultiplayerGame, this)
        )
    );
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
}

void MenuModeMain::connectModeChangeEvent(const std::string& buttonName, AbstractModeManager::ModeType mode)
{
    CEGUI::Window* window = getModeManager().getGui().getGuiSheet(Gui::mainMenu);

    addEventConnection(
        window->getChild(buttonName)->subscribeEvent(
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

bool MenuModeMain::openSkirmishSubMenu(const CEGUI::EventArgs&)
{
    CEGUI::Window* window = getModeManager().getGui().getGuiSheet(Gui::mainMenu)->getChild("SkirmishSubMenuWindow");
    window->show();
    window->setModalState(true);
    return true;
}

bool MenuModeMain::closeSkirmishSubMenu(const CEGUI::EventArgs&)
{
    CEGUI::Window* window = getModeManager().getGui().getGuiSheet(Gui::mainMenu)->getChild("SkirmishSubMenuWindow");
    window->setModalState(false);
    window->hide();
    return true;
}

bool MenuModeMain::startSkirmish(const CEGUI::EventArgs&)
{
    CEGUI::Window* window = getModeManager().getGui().getGuiSheet(Gui::mainMenu)->getChild("SkirmishSubMenuWindow");
    window->setModalState(false);
    window->hide();
    getModeManager().requestMode(AbstractModeManager::ModeType::MENU_SKIRMISH);
    return true;
}

bool MenuModeMain::loadSkirmish(const CEGUI::EventArgs&)
{
    CEGUI::Window* window = getModeManager().getGui().getGuiSheet(Gui::mainMenu)->getChild("SkirmishSubMenuWindow");
    window->setModalState(false);
    window->hide();
    getModeManager().requestMode(AbstractModeManager::ModeType::MENU_LOAD_SAVEDGAME);
    return true;
}

bool MenuModeMain::openMultiplayerSubMenu(const CEGUI::EventArgs&)
{
    CEGUI::Window* window = getModeManager().getGui().getGuiSheet(Gui::mainMenu)->getChild("MultiplayerSubMenuWindow");
    window->show();
    window->setModalState(true);
    return true;
}

bool MenuModeMain::closeMultiplayerSubMenu(const CEGUI::EventArgs&)
{
    CEGUI::Window* window = getModeManager().getGui().getGuiSheet(Gui::mainMenu)->getChild("MultiplayerSubMenuWindow");
    window->setModalState(false);
    window->hide();
    return true;
}

bool MenuModeMain::joinMasterServerGame(const CEGUI::EventArgs&)
{
    CEGUI::Window* window = getModeManager().getGui().getGuiSheet(Gui::mainMenu)->getChild("MultiplayerSubMenuWindow");
    window->setModalState(false);
    window->hide();
    getModeManager().requestMode(AbstractModeManager::ModeType::MENU_MASTERSERVER_JOIN);
    return true;
}

bool MenuModeMain::hostMasterServerGame(const CEGUI::EventArgs&)
{
    CEGUI::Window* window = getModeManager().getGui().getGuiSheet(Gui::mainMenu)->getChild("MultiplayerSubMenuWindow");
    window->setModalState(false);
    window->hide();
    getModeManager().requestMode(AbstractModeManager::ModeType::MENU_MASTERSERVER_HOST);
    return true;
}

bool MenuModeMain::joinMultiplayerGame(const CEGUI::EventArgs&)
{
    CEGUI::Window* window = getModeManager().getGui().getGuiSheet(Gui::mainMenu)->getChild("MultiplayerSubMenuWindow");
    window->setModalState(false);
    window->hide();
    getModeManager().requestMode(AbstractModeManager::ModeType::MENU_MULTIPLAYER_CLIENT);
    return true;
}

bool MenuModeMain::hostMultiplayerGame(const CEGUI::EventArgs&)
{
    CEGUI::Window* window = getModeManager().getGui().getGuiSheet(Gui::mainMenu)->getChild("MultiplayerSubMenuWindow");
    window->setModalState(false);
    window->hide();
    getModeManager().requestMode(AbstractModeManager::ModeType::MENU_MULTIPLAYER_SERVER);
    return true;
}
