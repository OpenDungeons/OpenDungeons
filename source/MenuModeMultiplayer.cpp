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

#include "MenuModeMultiplayer.h"

#include "Helper.h"
#include "Gui.h"
#include "ModeManager.h"
#include "MusicPlayer.h"
#include "GameMap.h"
#include "ODFrameListener.h"
#include "ODServer.h"
#include "ODClient.h"
#include "ODApplication.h"
#include "LogManager.h"

#include <CEGUI/CEGUI.h>
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>

const std::string LEVEL_PATH = "./levels/multiplayer/";
const std::string LEVEL_EXTENSION = ".level";

MenuModeMultiplayer::MenuModeMultiplayer(ModeManager *modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU_MULTIPLAYER)
{
}

MenuModeMultiplayer::~MenuModeMultiplayer()
{
}

void MenuModeMultiplayer::activate()
{
    // Loads the corresponding Gui sheet.
    Gui::getSingleton().loadGuiSheet(Gui::multiplayerMenu);

    giveFocus();

    // Play the main menu music
    // TODO: Make this configurable.
    MusicPlayer::getSingleton().play("Pal_Zoltan_Illes_OpenDungeons_maintheme.ogg");

    ODFrameListener::getSingleton().getClientGameMap()->setGamePaused(true);

    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_LIST_LEVELS);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_TEXT_LOADING);
    tmpWin->hide();
    mListFiles.clear();
    levelSelectList->resetList();

    if(Helper::fillFileStemsList(LEVEL_PATH, mListFiles, LEVEL_EXTENSION))
    {
        for (int n = 0; n < mListFiles.size(); ++n)
        {
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(mListFiles[n]);
            item->setID(n);
            item->setSelectionBrushImage("OpenDungeonsOldSkin/ListboxSelectionBrush");
            levelSelectList->addItem(item);
        }
    }
}

void MenuModeMultiplayer::serverButtonPressed()
{
    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_LIST_LEVELS);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    if(levelSelectList->getSelectedCount() == 0)
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("Please select a level first.");
        tmpWin->show();
        return;
    }

    tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_EDIT_NICK);
    CEGUI::Editbox* editNick = static_cast<CEGUI::Editbox*>(tmpWin);
    const std::string& str = editNick->getText().c_str();
    // Remove potential characters leading to crash.
    std::string nick = boost::locale::conv::to_utf<char>(str, "Ascii");
    if (nick.empty())
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("Please enter a nickname.");
        tmpWin->show();
        return;
    }
    else if (nick.length() > 20)
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("Please enter a shorter nickname. (20 letters max.)");
        tmpWin->show();
        return;
    }

    Player* p = ODFrameListener::getSingleton().getClientGameMap()->getLocalPlayer();
    p->setNick(nick);

    tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_TEXT_LOADING);
    tmpWin->setText("Loading...");
    tmpWin->show();

    CEGUI::ListboxItem* selItem = levelSelectList->getFirstSelectedItem();
    int id = selItem->getID();

    // We are a server
    std::string level = LEVEL_PATH + mListFiles[id] + LEVEL_EXTENSION;
    if(!ODServer::getSingleton().startServer(level, false, ODServer::ServerMode::ModeGame))
    {
        LogManager::getSingleton().logMessage("ERROR: Could not start server for multi player game !!!");
    }

    // We connect ourself
    if(ODClient::getSingleton().connect("", ODApplication::PORT_NUMBER))
    {
        mModeManager->requestGameMode(true);
    }
    else
    {
        LogManager::getSingleton().logMessage("ERROR: Could not connect to server for multi player game !!!");
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("Error: Couldn't connect to local server!");
        tmpWin->show();
    }
}

void MenuModeMultiplayer::clientButtonPressed()
{
    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_EDIT_IP);
    CEGUI::Editbox* editIp = static_cast<CEGUI::Editbox*>(tmpWin);
    const std::string ip = editIp->getText().c_str();

    if (ip.empty())
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("Please enter a server IP.");
        tmpWin->show();
        return;
    }

    tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_EDIT_NICK);
    CEGUI::Editbox* editNick = static_cast<CEGUI::Editbox*>(tmpWin);
    std::string str = editNick->getText().c_str();
    // Remove potential characters leading to crash.
    std::string nick = boost::locale::conv::to_utf<char>(str, "Ascii");
    if (nick.empty())
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("Please enter a nickname.");
        tmpWin->show();
        return;
    }
    else if (nick.length() > 20)
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("Please enter a shorter nickname. (20 letters max.)");
        tmpWin->show();
        return;
    }

    tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_TEXT_LOADING);
    tmpWin->setText("Loading...");
    tmpWin->show();

    ODFrameListener::getSingleton().getClientGameMap()->getLocalPlayer()->setNick(nick);

    if(ODClient::getSingleton().connect(ip, ODApplication::PORT_NUMBER))
    {
        mModeManager->requestGameMode();
    }
    else
    {
        // Error while connecting
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("Could not connect to: " + ip);
    }
}

void MenuModeMultiplayer::listLevelsClicked()
{
    serverButtonPressed();
}

bool MenuModeMultiplayer::mouseMoved(const OIS::MouseEvent &arg)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition((float)arg.state.X.abs, (float)arg.state.Y.abs);
}

bool MenuModeMultiplayer::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
        Gui::getSingletonPtr()->convertButton(id));
}

bool MenuModeMultiplayer::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
        Gui::getSingletonPtr()->convertButton(id));
}

bool MenuModeMultiplayer::keyPressed(const OIS::KeyEvent &arg)
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

bool MenuModeMultiplayer::keyReleased(const OIS::KeyEvent &arg)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyUp((CEGUI::Key::Scan) arg.key);

    return true;
}

void MenuModeMultiplayer::handleHotkeys(OIS::KeyCode keycode)
{
}
