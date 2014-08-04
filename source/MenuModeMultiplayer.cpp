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
#include "boost/filesystem.hpp"

const std::string MenuModeMultiplayer::LEVEL_PATH = "./levels/multiplayer/";
const std::string MenuModeMultiplayer::LEVEL_EXTENSION = ".level";

MenuModeMultiplayer::MenuModeMultiplayer(ModeManager *modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU_MULTIPLAYER)
{
}

MenuModeMultiplayer::~MenuModeMultiplayer()
{
}

bool MenuModeMultiplayer::fillFilesList(const std::string& path, std::vector<std::string>& listFiles)
{
    const boost::filesystem::path dir_path(path);
    if (!boost::filesystem::exists(dir_path))
        return false;
    boost::filesystem::directory_iterator end_itr;
    for (boost::filesystem::directory_iterator itr(dir_path); itr != end_itr; ++itr )
    {
        if(!boost::filesystem::is_directory(itr->status()))
        {
            if(itr->path().filename().extension().string() == LEVEL_EXTENSION)
                listFiles.push_back(itr->path().filename().stem().string());
        }
    }
    return true;
}

void MenuModeMultiplayer::activate()
{
    // Loads the corresponding Gui sheet.
    Gui::getSingleton().loadGuiSheet(Gui::multiplayerMenu);

    giveFocus();

    // Play the main menu music
    MusicPlayer::getSingleton().start(0);

    ODFrameListener::getSingleton().getGameMap()->setGamePaused(true);

    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_LIST_LEVELS);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_TEXT_LOADING);
    tmpWin->hide();
    listFiles.clear();
    levelSelectList->resetList();

    if(fillFilesList(LEVEL_PATH, listFiles))
    {
        for (int n = 0; n < listFiles.size(); n++)
        {
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(listFiles[n]);
            item->setID(n);
            item->setSelectionBrushImage("OpenDungeons/ListboxSelectionBrush");
            levelSelectList->addItem(item);
        }
    }
}

void MenuModeMultiplayer::serverButtonPressed()
{
    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_LIST_LEVELS);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    if(levelSelectList->getSelectedCount() > 0)
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("Loading");
        tmpWin->show();

        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_EDIT_NICK);
        CEGUI::Editbox* editNick = static_cast<CEGUI::Editbox*>(tmpWin);
        const std::string& nick = editNick->getText().c_str();
        Player *p = ODFrameListener::getSingleton().getGameMap()->getLocalPlayer();
        p->setNick(nick);


        CEGUI::ListboxItem*	selItem = levelSelectList->getFirstSelectedItem();
        int id = selItem->getID();

        // We are a server
        std::string level = LEVEL_PATH + listFiles[id] + LEVEL_EXTENSION;
        ODServer::getSingleton().startServer(level, false);

        // We connect ourself
        if(ODClient::getSingleton().connect("", ODApplication::PORT_NUMBER, level))
        {
            mModeManager->requestGameMode(true);
        }
        else
        {
            LogManager::getSingleton().logMessage("ERROR: Could not connect to server for single player game !!!");
        }
    }
}

void MenuModeMultiplayer::clientButtonPressed()
{
    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_LIST_LEVELS);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    if(levelSelectList->getSelectedCount() > 0)
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("Loading");
        tmpWin->show();

        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_EDIT_IP);
        CEGUI::Editbox* editIp = static_cast<CEGUI::Editbox*>(tmpWin);
        const std::string ip = editIp->getText().c_str();

        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_EDIT_NICK);
        CEGUI::Editbox* editNick = static_cast<CEGUI::Editbox*>(tmpWin);
        const std::string nick = editNick->getText().c_str();
        ODFrameListener::getSingleton().getGameMap()->getLocalPlayer()->setNick(nick);

        CEGUI::ListboxItem*	selItem = levelSelectList->getFirstSelectedItem();
        int id = selItem->getID();

        std::string level = LEVEL_PATH + listFiles[id] + LEVEL_EXTENSION;
        if(ODClient::getSingleton().connect(ip, ODApplication::PORT_NUMBER, level))
        {
            mModeManager->requestGameMode();
        }
        else
        {
            // Error while connecting
            tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerMenu)->getChild(Gui::MPM_TEXT_LOADING);
            tmpWin->setText("Could not connect to " + ip);
        }
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

bool MenuModeMultiplayer::keyPressed(const OIS::KeyEvent &arg) {
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
