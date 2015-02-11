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

#include "modes/MenuModeMultiplayerServer.h"

#include "utils/Helper.h"
#include "render/Gui.h"
#include "modes/ModeManager.h"
#include "sound/MusicPlayer.h"
#include "gamemap/GameMap.h"
#include "render/ODFrameListener.h"
#include "network/ODServer.h"
#include "network/ODClient.h"
#include "ODApplication.h"
#include "utils/LogManager.h"
#include "gamemap/MapLoader.h"
#include "utils/ConfigManager.h"
#include "utils/ResourceManager.h"

#include <CEGUI/CEGUI.h>
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>

const std::string LEVEL_PATH = "levels/multiplayer/";
const std::string LEVEL_EXTENSION = ".level";

MenuModeMultiplayerServer::MenuModeMultiplayerServer(ModeManager *modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU_MULTIPLAYER_SERVER)
{
}

MenuModeMultiplayerServer::~MenuModeMultiplayerServer()
{
}

void MenuModeMultiplayerServer::activate()
{
    // Loads the corresponding Gui sheet.
    Gui::getSingleton().loadGuiSheet(Gui::multiplayerServerMenu);

    giveFocus();

    // Play the main menu music
    // TODO: Make this configurable.
    MusicPlayer::getSingleton().play("Pal_Zoltan_Illes_OpenDungeons_maintheme.ogg");

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->clearAll();
    gameMap->processDeletionQueues();
    gameMap->setGamePaused(true);

    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerServerMenu)->getChild(Gui::MPM_LIST_LEVELS);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerServerMenu)->getChild(Gui::MPM_TEXT_LOADING);
    tmpWin->hide();
    mFilesList.clear();
    mDescriptionList.clear();
    levelSelectList->resetList();

    std::string levelPath = ResourceManager::getSingleton().getGameDataPath() + LEVEL_PATH;
    if(Helper::fillFilesList(levelPath, mFilesList, LEVEL_EXTENSION))
    {
        for (uint32_t n = 0; n < mFilesList.size(); ++n)
        {
            std::string filename = mFilesList[n];

            LevelInfo levelInfo;
            std::string mapName;
            std::string mapDescription;
            if(MapLoader::getMapInfo(filename, levelInfo))
            {
                mapName = levelInfo.mLevelName;
                mapDescription = levelInfo.mLevelDescription;
            }
            else
            {
                mapName = "invalid map";
                mapDescription = "invalid map";
            }

            mDescriptionList.push_back(mapDescription);
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(mapName);
            item->setID(n);
            item->setSelectionBrushImage("OpenDungeonsSkin/SelectionBrush");
            levelSelectList->addItem(item);

            // We reconstruct the filename to be relative to the levels/ folder
            // because we'll need a relative reference for the clients.
            std::string levelFile = LEVEL_PATH + boost::filesystem::path(mFilesList[n]).filename().string();
            mFilesList[n] = levelFile;
        }
    }
}

void MenuModeMultiplayerServer::serverButtonPressed()
{
    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerServerMenu)->getChild(Gui::MPM_LIST_LEVELS);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    if(levelSelectList->getSelectedCount() == 0)
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerServerMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("Please select a level first.");
        tmpWin->show();
        return;
    }

    tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerServerMenu)->getChild(Gui::MPM_EDIT_NICK);
    CEGUI::Editbox* editNick = static_cast<CEGUI::Editbox*>(tmpWin);
    const std::string& str = editNick->getText().c_str();
    // Remove potential characters leading to crash.
    std::string nick = boost::locale::conv::to_utf<char>(str, "Ascii");
    if (nick.empty())
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerServerMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("Please enter a nickname.");
        tmpWin->show();
        return;
    }
    else if (nick.length() > 20)
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerServerMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("Please enter a shorter nickname. (20 letters max.)");
        tmpWin->show();
        return;
    }

    ODFrameListener::getSingleton().getClientGameMap()->setLocalPlayerNick(nick);

    tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerServerMenu)->getChild(Gui::MPM_TEXT_LOADING);
    tmpWin->setText("Loading...");
    tmpWin->show();

    CEGUI::ListboxItem* selItem = levelSelectList->getFirstSelectedItem();
    int id = selItem->getID();

    std::string level = mFilesList[id];

    // We are a server
    if(!ODServer::getSingleton().startServer(level, ODServer::ServerMode::ModeGameMultiPlayer))
    {
        LogManager::getSingleton().logMessage("ERROR: Could not start server for multi player game !!!");
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerServerMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("ERROR: Could not start server for multi player game !!!");
        tmpWin->show();
        return;
    }

    // We connect ourself
    if(!ODClient::getSingleton().connect("localhost", ConfigManager::getSingleton().getNetworkPort()))
    {
        LogManager::getSingleton().logMessage("ERROR: Could not connect to server for multi player game !!!");
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerServerMenu)->getChild(Gui::MPM_TEXT_LOADING);
        tmpWin->setText("Error: Couldn't connect to local server!");
        tmpWin->show();
        return;
    }
}

void MenuModeMultiplayerServer::updateDescription()
{
    // Get the level corresponding id
    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::multiplayerServerMenu)->getChild(Gui::SKM_LIST_LEVELS);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    CEGUI::Window* descTxt = Gui::getSingleton().getGuiSheet(Gui::multiplayerServerMenu)->getChild("LevelWindowFrame/MapDescriptionText");

    if(levelSelectList->getSelectedCount() == 0)
    {
        descTxt->setText("");
        return;
    }

    CEGUI::ListboxItem* selItem = levelSelectList->getFirstSelectedItem();
    int id = selItem->getID();

    std::string description = mDescriptionList[id];
    descTxt->setText(description);
}

void MenuModeMultiplayerServer::listLevelsClicked()
{
    updateDescription();
}

void MenuModeMultiplayerServer::listLevelsDoubleClicked()
{
    serverButtonPressed();
}
