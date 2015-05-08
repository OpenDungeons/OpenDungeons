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

#include "modes/MenuModeLoad.h"

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

const std::string SAVEGAME_EXTENSION = ".level";

MenuModeLoad::MenuModeLoad(ModeManager *modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU_LOAD_SAVEDGAME)
{
    CEGUI::Window* window = modeManager->getGui().getGuiSheet(Gui::guiSheet::loadSavedGameMenu);
    addEventConnection(
        window->getChild("LevelWindowFrame/BackButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeLoad::goBack, this)
        )
    );
    addEventConnection(
        window->getChild("LevelWindowFrame/LaunchButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeLoad::launchSelectedButtonPressed, this)
        )
    );
    addEventConnection(
        window->getChild("LevelWindowFrame/DeleteButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeLoad::deleteSelectedButtonPressed, this)
        )
    );
    addEventConnection(
        window->getChild("LevelWindowFrame/SaveGameSelect")->subscribeEvent(
            CEGUI::Listbox::EventMouseClick,
            CEGUI::Event::Subscriber(&MenuModeLoad::updateDescription, this)
        )
    );
    addEventConnection(
        window->getChild("LevelWindowFrame/SaveGameSelect")->subscribeEvent(
            CEGUI::Listbox::EventMouseDoubleClick,
            CEGUI::Event::Subscriber(&MenuModeLoad::launchSelectedButtonPressed, this)
        )
    );
    addEventConnection(
        window->getChild("LevelWindowFrame/__auto_closebutton__")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeLoad::goBack, this)
        )
    );
}

void MenuModeLoad::activate()
{
    // Loads the corresponding Gui sheet.
    getModeManager().getGui().loadGuiSheet(Gui::guiSheet::loadSavedGameMenu);

    giveFocus();

    // Play the main menu music
    // TODO: Make this configurable.
    MusicPlayer::getSingleton().play("OpenDungeonsMainTheme_pZi.ogg");


    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->clearAll();
    gameMap->setGamePaused(true);

    CEGUI::Window* tmpWin = getModeManager().getGui().getGuiSheet(Gui::loadSavedGameMenu)->getChild("LevelWindowFrame/SaveGameSelect");
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    tmpWin = getModeManager().getGui().getGuiSheet(Gui::loadSavedGameMenu)->getChild("LoadingText");
    tmpWin->hide();
    mFilesList.clear();
    levelSelectList->resetList();

    std::string levelPath = ResourceManager::getSingleton().getSaveGamePath();
    if(Helper::fillFilesList(levelPath, mFilesList, SAVEGAME_EXTENSION))
    {
        for (uint32_t n = 0; n < mFilesList.size(); ++n)
        {
            std::string filename = boost::filesystem::path(mFilesList[n]).filename().string();
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(filename);
            item->setID(n);
            item->setSelectionBrushImage("OpenDungeonsSkin/SelectionBrush");
            levelSelectList->addItem(item);
        }
    }
}

bool MenuModeLoad::launchSelectedButtonPressed(const CEGUI::EventArgs&)
{
    CEGUI::Window* tmpWin = getModeManager().getGui().getGuiSheet(Gui::loadSavedGameMenu)->getChild("LevelWindowFrame/SaveGameSelect");
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    if(levelSelectList->getSelectedCount() == 0)
    {
        tmpWin = getModeManager().getGui().getGuiSheet(Gui::loadSavedGameMenu)->getChild("LoadingText");
        tmpWin->setText("Please select a saved game first.");
        tmpWin->show();
        return true;
    }

    tmpWin = getModeManager().getGui().getGuiSheet(Gui::loadSavedGameMenu)->getChild("LoadingText");
    tmpWin->setText("Loading...");
    tmpWin->show();

    CEGUI::ListboxItem* selItem = levelSelectList->getFirstSelectedItem();
    int id = selItem->getID();

    const std::string& level = mFilesList[id];
    // In single player mode, we act as a server
    if(!ODServer::getSingleton().startServer(level, ServerMode::ModeGameLoaded))
    {
        LogManager::getSingleton().logMessage("ERROR: Could not start server for single player game !!!");
        tmpWin = getModeManager().getGui().getGuiSheet(Gui::loadSavedGameMenu)->getChild("LoadingText");
        tmpWin->setText("ERROR: Could not start server for single player game !!!");
        tmpWin->show();
        return true;
    }

    if(!ODClient::getSingleton().connect("localhost", ConfigManager::getSingleton().getNetworkPort()))
    {
        LogManager::getSingleton().logMessage("ERROR: Could not connect to server for single player game !!!");
        tmpWin = getModeManager().getGui().getGuiSheet(Gui::loadSavedGameMenu)->getChild("LoadingText");
        tmpWin->setText("Error: Couldn't connect to local server!");
        tmpWin->show();
        return true;
    }
    return true;
}

bool MenuModeLoad::deleteSelectedButtonPressed(const CEGUI::EventArgs&)
{
    CEGUI::Window* tmpWin = getModeManager().getGui().getGuiSheet(Gui::loadSavedGameMenu)->getChild("LevelWindowFrame/SaveGameSelect");
    CEGUI::Listbox* selectList = static_cast<CEGUI::Listbox*>(tmpWin);

    if(selectList->getSelectedCount() == 0)
    {
        tmpWin = getModeManager().getGui().getGuiSheet(Gui::loadSavedGameMenu)->getChild("LoadingText");
        tmpWin->setText("Please select a saved game first.");
        tmpWin->show();
        return true;
    }

    CEGUI::ListboxItem* selItem = selectList->getFirstSelectedItem();
    int id = selItem->getID();

    const std::string& levelFile = mFilesList[id];
    if(!boost::filesystem::remove(levelFile))
    {
        LogManager::getSingleton().logMessage("ERROR: Could not delete saved game !!!");
        tmpWin = getModeManager().getGui().getGuiSheet(Gui::loadSavedGameMenu)->getChild("LoadingText");
        tmpWin->setText("Error: Couldn't delete saved game!");
        tmpWin->show();
        return true;
    }

    activate();
    return true;
}

bool MenuModeLoad::updateDescription(const CEGUI::EventArgs&)
{
    // Get the level corresponding id
    CEGUI::Window* tmpWin = getModeManager().getGui().getGuiSheet(Gui::loadSavedGameMenu)->getChild("LevelWindowFrame/SaveGameSelect");
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    CEGUI::Window* descTxt = getModeManager().getGui().getGuiSheet(Gui::loadSavedGameMenu)->getChild("LevelWindowFrame/MapDescriptionText");

    if(levelSelectList->getSelectedCount() == 0)
    {
        descTxt->setText("");
        return true;
    }

    getModeManager().getGui().playButtonClickSound();

    CEGUI::ListboxItem* selItem = levelSelectList->getFirstSelectedItem();
    int id = selItem->getID();

    std::string filename = mFilesList[id];

    LevelInfo levelInfo;
    std::string mapDescription;
    if(MapLoader::getMapInfo(filename, levelInfo))
        mapDescription = levelInfo.mLevelDescription;
    else
        mapDescription = "invalid map";

    descTxt->setText(mapDescription);

    return true;
}

bool MenuModeLoad::goBack(const CEGUI::EventArgs &)
{
    getModeManager().requestMode(AbstractModeManager::MAIN_MENU);
    return true;
}
