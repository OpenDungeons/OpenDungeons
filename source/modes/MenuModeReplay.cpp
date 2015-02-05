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

#include "modes/MenuModeReplay.h"

#include "utils/Helper.h"
#include "render/Gui.h"
#include "modes/ModeManager.h"
#include "sound/MusicPlayer.h"
#include "gamemap/GameMap.h"
#include "render/ODFrameListener.h"
#include "network/ODServer.h"
#include "network/ODClient.h"
#include "network/ServerNotification.h"
#include "ODApplication.h"
#include "utils/LogManager.h"
#include "gamemap/MapLoader.h"
#include "utils/ConfigManager.h"
#include "utils/ResourceManager.h"

#include <CEGUI/CEGUI.h>
#include "boost/filesystem.hpp"

const std::string REPLAY_EXTENSION = ".odr";

MenuModeReplay::MenuModeReplay(ModeManager *modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU_REPLAY)
{
}

MenuModeReplay::~MenuModeReplay()
{
}

void MenuModeReplay::activate()
{
    // Loads the corresponding Gui sheet.
    Gui::getSingleton().loadGuiSheet(Gui::replayMenu);

    giveFocus();

    // Play the main menu music
    // TODO: Make this configurable.
    MusicPlayer::getSingleton().play("Pal_Zoltan_Illes_OpenDungeons_maintheme.ogg");


    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->clearAll();
    gameMap->processDeletionQueues();
    gameMap->setGamePaused(true);

    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_LIST_REPLAYS);
    CEGUI::Listbox* replaySelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    tmpWin = Gui::getSingleton().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_TEXT_LOADING);
    tmpWin->hide();
    mFilesList.clear();
    replaySelectList->resetList();

    std::string replayPath = ResourceManager::getSingleton().getReplayDataPath();
    if(Helper::fillFilesList(replayPath, mFilesList, REPLAY_EXTENSION))
    {
        for (uint32_t n = 0; n < mFilesList.size(); ++n)
        {
            std::string filename = boost::filesystem::path(mFilesList[n]).filename().string();
            mFilesList[n] = filename;
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(filename);
            item->setID(n);
            item->setSelectionBrushImage("OpenDungeonsSkin/SelectionBrush");
            replaySelectList->addItem(item);
        }
    }
}

void MenuModeReplay::launchSelectedButtonPressed()
{
    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_LIST_REPLAYS);
    CEGUI::Listbox* replaySelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    if(replaySelectList->getSelectedCount() == 0)
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_TEXT_LOADING);
        tmpWin->setText("Please select a replay first.");
        tmpWin->show();
        return;
    }

    tmpWin = Gui::getSingleton().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_TEXT_LOADING);
    tmpWin->setText("Loading...");
    tmpWin->show();

    CEGUI::ListboxItem* selItem = replaySelectList->getFirstSelectedItem();
    int id = selItem->getID();

    std::string mapDescription;
    std::string errorMsg;
    if(!checkReplayValid(mFilesList[id], mapDescription, errorMsg))
    {
        tmpWin->setText("Error: trying to launch invalid replay!");
        tmpWin->show();
        return;
    }

    std::string replayFile = ResourceManager::getSingleton().getReplayDataPath() + mFilesList[id];
    if(!ODClient::getSingleton().replay(replayFile))
    {
        LogManager::getSingleton().logMessage("ERROR: Could not launch replay !!!");
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_TEXT_LOADING);
        tmpWin->setText("Error: Couldn't launch replay!");
        tmpWin->show();
        return;
    }
}

void MenuModeReplay::deleteSelectedButtonPressed()
{
    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_LIST_REPLAYS);
    CEGUI::Listbox* replaySelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    if(replaySelectList->getSelectedCount() == 0)
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_TEXT_LOADING);
        tmpWin->setText("Please select a replay first.");
        tmpWin->show();
        return;
    }

    CEGUI::ListboxItem* selItem = replaySelectList->getFirstSelectedItem();
    int id = selItem->getID();

    std::string replayFile = ResourceManager::getSingleton().getReplayDataPath() + mFilesList[id];
    if(!boost::filesystem::remove(replayFile))
    {
        LogManager::getSingleton().logMessage("ERROR: Could not delete replay !!!");
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_TEXT_LOADING);
        tmpWin->setText("Error: Couldn't delete replay!");
        tmpWin->show();
        return;
    }

    activate();
}

void MenuModeReplay::listReplaysClicked()
{
    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_LIST_REPLAYS);
    CEGUI::Listbox* replaySelectList = static_cast<CEGUI::Listbox*>(tmpWin);
    if(replaySelectList->getSelectedCount() == 0)
        return;

    CEGUI::ListboxItem* selItem = replaySelectList->getFirstSelectedItem();
    int id = selItem->getID();

    CEGUI::Window* descTxt = Gui::getSingleton().getGuiSheet(Gui::replayMenu)->getChild("LevelWindowFrame/MapDescriptionText");
    std::string mapDescription;
    std::string errorMsg;
    if(checkReplayValid(mFilesList[id], mapDescription, errorMsg))
    {
        descTxt->setText(mapDescription);
    }
    else
    {
        descTxt->setText(errorMsg);
    }
}

bool MenuModeReplay::checkReplayValid(const std::string& replayFileName, std::string& mapDescription, std::string& errorMsg)
{
    std::string replayFile = ResourceManager::getSingleton().getReplayDataPath() + replayFileName;
    // We open the replay to get the level file name
    std::ifstream is(replayFile, std::ios::in | std::ios::binary);
    ODPacket packet;
    ServerNotificationType type;
    do
    {
        packet.readPacket(is);
        OD_ASSERT_TRUE(packet >> type);
        if(is.eof())
            break;
    } while(type != ServerNotificationType::loadLevel);

    if(is.eof())
    {
        errorMsg = "Invalid replay file";
        return false;
    }

    std::string odVersion;
    std::string tmpStr;
    int32_t tmpInt;
    // OD version
    OD_ASSERT_TRUE(packet >> odVersion);
    // mapSizeX
    OD_ASSERT_TRUE(packet >> tmpInt);
    // mapSizeY
    OD_ASSERT_TRUE(packet >> tmpInt);
    // LevelFileName
    OD_ASSERT_TRUE(packet >> tmpStr);
    // LevelDescription
    OD_ASSERT_TRUE(packet >> mapDescription);

    if(odVersion.compare(std::string("OpenDungeons V ") + ODApplication::VERSION) != 0)
    {
        errorMsg = odVersion + " (Wrong version)\n\n" + mapDescription;
        return false;
    }

    return true;
}

void MenuModeReplay::listReplaysDoubleClicked()
{
    launchSelectedButtonPressed();
}
