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

#include "modes/MenuModeMultiplayerClient.h"

#include "utils/Helper.h"
#include "render/Gui.h"
#include "modes/ModeManager.h"
#include "sound/MusicPlayer.h"
#include "gamemap/GameMap.h"
#include "render/ODFrameListener.h"
#include "network/ODServer.h"
#include "network/ODClient.h"
#include "utils/ConfigManager.h"
#include "utils/LogManager.h"

#include <CEGUI/CEGUI.h>
#include <boost/locale.hpp>

MenuModeMultiplayerClient::MenuModeMultiplayerClient(ModeManager *modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU_MULTIPLAYER_CLIENT)
{
    CEGUI::Window* window = getModeManager().getGui().getGuiSheet(Gui::guiSheet::multiplayerClientMenu);

    addEventConnection(
        window->getChild(Gui::MPM_BUTTON_CLIENT)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeMultiplayerClient::clientButtonPressed,
                                     this)
        )
    );
    addEventConnection(
        window->getChild(Gui::MPM_BUTTON_BACK)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&AbstractApplicationMode::goBack,
                                     static_cast<AbstractApplicationMode*>(this))
        )
    );
    addEventConnection(
        window->getChild("LevelWindowFrame/__auto_closebutton__")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&AbstractApplicationMode::goBack,
                                     static_cast<AbstractApplicationMode*>(this))
        )
    );
}

void MenuModeMultiplayerClient::activate()
{
    // Loads the corresponding Gui sheet.
    Gui& gui = getModeManager().getGui();
    gui.loadGuiSheet(Gui::guiSheet::multiplayerClientMenu);

    giveFocus();

    // Play the main menu music
    // TODO: Make this configurable.
    MusicPlayer::getSingleton().play("OpenDungeonsMainTheme_pZi.ogg");

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->clearAll();
    gameMap->setGamePaused(true);

    CEGUI::Window* mainWin = gui.getGuiSheet(Gui::guiSheet::multiplayerClientMenu);
    mainWin->getChild(Gui::MPM_TEXT_LOADING)->setText("");

    CEGUI::Editbox* editNick = static_cast<CEGUI::Editbox*>(mainWin->getChild(Gui::MPM_EDIT_NICK));
    ConfigManager& config = ConfigManager::getSingleton();
    editNick->setText(reinterpret_cast<const CEGUI::utf8*>(config.getGameValue(Config::NICKNAME).c_str()));
}

bool MenuModeMultiplayerClient::clientButtonPressed(const CEGUI::EventArgs&)
{
    CEGUI::Window* mainWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::multiplayerClientMenu);
    CEGUI::Editbox* editIp = static_cast<CEGUI::Editbox*>(mainWin->getChild(Gui::MPM_EDIT_IP));
    const std::string ip = editIp->getText().c_str();

    CEGUI::Window* infoText = mainWin->getChild(Gui::MPM_TEXT_LOADING);

    if (ip.empty())
    {
        infoText->setText("Please enter a server IP.");
        return true;
    }

    CEGUI::Editbox* editNick = static_cast<CEGUI::Editbox*>(mainWin->getChild(Gui::MPM_EDIT_NICK));
    std::string nick = editNick->getText().c_str();
    CEGUI::String nickCeguiStr = reinterpret_cast<const CEGUI::utf8*>(nick.c_str());
    if (nickCeguiStr.empty())
    {
        infoText->setText("Please enter a nickname.");
        return true;
    }
    else if (nickCeguiStr.length() > 20)
    {
        infoText->setText("Please enter a shorter nickname. (20 letters max.)");
        return true;
    }

    ODFrameListener::getSingleton().getClientGameMap()->setLocalPlayerNick(nick);

    if(!ODClient::getSingleton().connect(ip, ConfigManager::getSingleton().getNetworkPort()))
    {
        // Error while connecting
        infoText->setText("Could not connect to: " + ip);
        return true;
    }

    infoText->setText("Loading...");
    return true;
}
