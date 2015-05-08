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
    CEGUI::Window* window = modeManager->getGui().getGuiSheet(Gui::guiSheet::replayMenu);
    addEventConnection(
        window->getChild(Gui::REM_BUTTON_BACK)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeReplay::goBack, this)
        )
    );
    addEventConnection(
        window->getChild(Gui::REM_BUTTON_LAUNCH)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeReplay::launchSelectedButtonPressed, this)
        )
    );
    addEventConnection(
        window->getChild(Gui::REM_BUTTON_DELETE)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeReplay::deleteSelectedButtonPressed, this)
        )
    );
    addEventConnection(
        window->getChild(Gui::REM_LIST_REPLAYS)->subscribeEvent(
            CEGUI::Listbox::EventMouseClick,
            CEGUI::Event::Subscriber(&MenuModeReplay::listReplaysClicked, this)
        )
    );
    addEventConnection(
        window->getChild(Gui::REM_LIST_REPLAYS)->subscribeEvent(
            CEGUI::Listbox::EventMouseDoubleClick,
            CEGUI::Event::Subscriber(&MenuModeReplay::launchSelectedButtonPressed, this)
        )
    );
    addEventConnection(
        window->getChild("LevelWindowFrame/__auto_closebutton__")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeReplay::goBack, this)
        )
    );
}

void MenuModeReplay::activate()
{
    // Loads the corresponding Gui sheet.
    getModeManager().getGui().loadGuiSheet(Gui::replayMenu);

    giveFocus();

    // Play the main menu music
    // TODO: Make this configurable.
    MusicPlayer::getSingleton().play("OpenDungeonsMainTheme_pZi.ogg");


    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->clearAll();
    gameMap->setGamePaused(true);

    CEGUI::Window* tmpWin = getModeManager().getGui().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_LIST_REPLAYS);
    CEGUI::Listbox* replaySelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    tmpWin = getModeManager().getGui().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_TEXT_LOADING);
    tmpWin->hide();
    mFilesList.clear();
    replaySelectList->resetList();

    std::string replayPath = ResourceManager::getSingleton().getReplayDataPath();
    if(Helper::fillFilesList(replayPath, mFilesList, REPLAY_EXTENSION))
    {
        for (uint32_t n = 0; n < mFilesList.size(); ++n)
        {
            std::string filename = boost::filesystem::path(mFilesList[n]).filename().string();
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(filename);
            item->setID(n);
            item->setSelectionBrushImage("OpenDungeonsSkin/SelectionBrush");
            replaySelectList->addItem(item);
        }
    }
}

bool MenuModeReplay::launchSelectedButtonPressed(const CEGUI::EventArgs&)
{
    CEGUI::Window* tmpWin = getModeManager().getGui().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_LIST_REPLAYS);
    CEGUI::Listbox* replaySelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    if(replaySelectList->getSelectedCount() == 0)
    {
        tmpWin = getModeManager().getGui().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_TEXT_LOADING);
        tmpWin->setText("Please select a replay first.");
        tmpWin->show();
        return true;
    }

    tmpWin = getModeManager().getGui().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_TEXT_LOADING);
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
        return true;
    }

    const std::string& replayFile = mFilesList[id];
    if(!ODClient::getSingleton().replay(replayFile))
    {
        LogManager::getSingleton().logMessage("ERROR: Could not launch replay !!!");
        tmpWin = getModeManager().getGui().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_TEXT_LOADING);
        tmpWin->setText("Error: Couldn't launch replay!");
        tmpWin->show();
        return true;
    }
    return true;
}

bool MenuModeReplay::deleteSelectedButtonPressed(const CEGUI::EventArgs&)
{
    CEGUI::Window* tmpWin = getModeManager().getGui().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_LIST_REPLAYS);
    CEGUI::Listbox* replaySelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    if(replaySelectList->getSelectedCount() == 0)
    {
        tmpWin = getModeManager().getGui().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_TEXT_LOADING);
        tmpWin->setText("Please select a replay first.");
        tmpWin->show();
        return true;
    }

    CEGUI::ListboxItem* selItem = replaySelectList->getFirstSelectedItem();
    int id = selItem->getID();

    const std::string& replayFile = mFilesList[id];
    if(!boost::filesystem::remove(replayFile))
    {
        LogManager::getSingleton().logMessage("ERROR: Could not delete replay !!!");
        tmpWin = getModeManager().getGui().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_TEXT_LOADING);
        tmpWin->setText("Error: Couldn't delete replay!");
        tmpWin->show();
        return true;
    }

    activate();
    return true;
}

bool MenuModeReplay::listReplaysClicked(const CEGUI::EventArgs&)
{
    CEGUI::Window* tmpWin = getModeManager().getGui().getGuiSheet(Gui::replayMenu)->getChild(Gui::REM_LIST_REPLAYS);
    CEGUI::Listbox* replaySelectList = static_cast<CEGUI::Listbox*>(tmpWin);
    if(replaySelectList->getSelectedCount() == 0)
        return true;

    getModeManager().getGui().playButtonClickSound();

    CEGUI::ListboxItem* selItem = replaySelectList->getFirstSelectedItem();
    int id = selItem->getID();

    CEGUI::Window* descTxt = getModeManager().getGui().getGuiSheet(Gui::replayMenu)->getChild("LevelWindowFrame/MapDescriptionText");
    std::string mapDescription;
    std::string errorMsg;
    if(checkReplayValid(mFilesList[id], mapDescription, errorMsg))
    {
        descTxt->setText(reinterpret_cast<const CEGUI::utf8*>(mapDescription.c_str()));
    }
    else
    {
        descTxt->setText(reinterpret_cast<const CEGUI::utf8*>(errorMsg.c_str()));
    }
    return true;
}

bool MenuModeReplay::checkReplayValid(const std::string& replayFileName, std::string& mapDescription, std::string& errorMsg)
{
    // We open the replay to get the level file name
    std::ifstream is(replayFileName, std::ios::in | std::ios::binary);
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

bool MenuModeReplay::goBack(const CEGUI::EventArgs &)
{
    getModeManager().requestMode(AbstractModeManager::MAIN_MENU);
    return true;
}
