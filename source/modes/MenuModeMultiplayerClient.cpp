/*
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

#include "modes/MenuModeMultiplayerClient.h"

#include "utils/Helper.h"
#include "render/Gui.h"
#include "modes/ModeManager.h"
#include "sound/MusicPlayer.h"
#include "gamemap/GameMap.h"
#include "render/ODFrameListener.h"
#include "network/ODServer.h"
#include "network/ODClient.h"
#include "ODApplication.h"
#include "utils/ConfigManager.h"
#include "utils/LogManager.h"

#include <CEGUI/CEGUI.h>
#include <boost/locale.hpp>

MenuModeMultiplayerClient::MenuModeMultiplayerClient(ModeManager *modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU_MULTIPLAYER_CLIENT)
{
}

MenuModeMultiplayerClient::~MenuModeMultiplayerClient()
{
}

void MenuModeMultiplayerClient::activate()
{
    // Loads the corresponding Gui sheet.
    Gui::getSingleton().loadGuiSheet(Gui::multiplayerClientMenu);

    giveFocus();

    // Play the main menu music
    // TODO: Make this configurable.
    MusicPlayer::getSingleton().play("Pal_Zoltan_Illes_OpenDungeons_maintheme.ogg");

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->clearAll();
    gameMap->processDeletionQueues();
    gameMap->setGamePaused(true);

    CEGUI::Window* mainWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerClientMenu);
    mainWin->getChild(Gui::MPM_TEXT_LOADING)->hide();
}

void MenuModeMultiplayerClient::clientButtonPressed()
{
    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerClientMenu)->getChild(Gui::MPM_EDIT_IP);
    CEGUI::Editbox* editIp = static_cast<CEGUI::Editbox*>(tmpWin);
    const std::string ip = editIp->getText().c_str();

    if (ip.empty())
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerClientMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("Please enter a server IP.");
        tmpWin->show();
        return;
    }

    tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerClientMenu)->getChild(Gui::MPM_EDIT_NICK);
    CEGUI::Editbox* editNick = static_cast<CEGUI::Editbox*>(tmpWin);
    std::string str = editNick->getText().c_str();
    // Remove potential characters leading to crash.
    std::string nick = boost::locale::conv::to_utf<char>(str, "Ascii");
    if (nick.empty())
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerClientMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("Please enter a nickname.");
        tmpWin->show();
        return;
    }
    else if (nick.length() > 20)
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerClientMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("Please enter a shorter nickname. (20 letters max.)");
        tmpWin->show();
        return;
    }

    tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerClientMenu)->getChild(Gui::MPM_TEXT_LOADING);
    tmpWin->setText("Loading...");
    tmpWin->show();

    ODFrameListener::getSingleton().getClientGameMap()->setLocalPlayerNick(nick);

    if(!ODClient::getSingleton().connect(ip, ConfigManager::getSingleton().getNetworkPort()))
    {
        // Error while connecting
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerClientMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("Could not connect to: " + ip);
        return;
    }
}

bool MenuModeMultiplayerClient::mouseMoved(const OIS::MouseEvent &arg)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition((float)arg.state.X.abs, (float)arg.state.Y.abs);
}

bool MenuModeMultiplayerClient::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
        Gui::getSingletonPtr()->convertButton(id));
}

bool MenuModeMultiplayerClient::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
        Gui::getSingletonPtr()->convertButton(id));
}

bool MenuModeMultiplayerClient::keyPressed(const OIS::KeyEvent &arg)
{
    switch (arg.key)
    {

    case OIS::KC_ESCAPE:
        regressMode();
        break;
    default:
        CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyDown((CEGUI::Key::Scan) arg.key);
        CEGUI::System::getSingleton().getDefaultGUIContext().injectChar(arg.text);
        break;
    }
    return true;
}

bool MenuModeMultiplayerClient::keyReleased(const OIS::KeyEvent &arg)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyUp((CEGUI::Key::Scan) arg.key);

    return true;
}

void MenuModeMultiplayerClient::handleHotkeys(OIS::KeyCode keycode)
{
}
