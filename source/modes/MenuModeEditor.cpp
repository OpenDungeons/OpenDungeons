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

#include "modes/MenuModeEditor.h"

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
#include "utils/ResourceManager.h"
#include "utils/ConfigManager.h"

#include <CEGUI/CEGUI.h>

#include <boost/filesystem.hpp>

const std::string LEVEL_PATH_SKIRMISH = "levels/skirmish/";
const std::string LEVEL_PATH_MULTIPLAYER = "levels/multiplayer/";
const std::string LEVEL_EXTENSION = ".level";

MenuModeEditor::MenuModeEditor(ModeManager *modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU_EDITOR)
{
    CEGUI::Window* window = modeManager->getGui().getGuiSheet(Gui::guiSheet::editorMenu);
    addEventConnection(
        window->getChild(Gui::EDM_BUTTON_BACK)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeEditor::goBack, this)
        )
    );
    addEventConnection(
        window->getChild(Gui::EDM_BUTTON_LAUNCH)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeEditor::launchSelectedButtonPressed, this)
        )
    );
    addEventConnection(
        window->getChild(Gui::EDM_LIST_LEVELS)->subscribeEvent(
            CEGUI::Listbox::EventMouseDoubleClick,
            CEGUI::Event::Subscriber(&MenuModeEditor::launchSelectedButtonPressed, this)
        )
    );
    addEventConnection(
        window->getChild(Gui::EDM_LIST_LEVELS)->subscribeEvent(
            CEGUI::Listbox::EventMouseClick,
            CEGUI::Event::Subscriber(&MenuModeEditor::updateDescription, this)
        )
    );
    addEventConnection(
        window->getChild("LevelWindowFrame/__auto_closebutton__")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeEditor::goBack, this)
        )
    );
}

void MenuModeEditor::activate()
{
    // Loads the corresponding Gui sheet.
    getModeManager().getGui().loadGuiSheet(Gui::editorMenu);

    giveFocus();

    // Play the main menu music
    // TODO: Make this configurable.
    MusicPlayer::getSingleton().play("OpenDungeonsMainTheme_pZi.ogg");

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->clearAll();
    gameMap->setGamePaused(true);

    CEGUI::Window* tmpWin = getModeManager().getGui().getGuiSheet(Gui::editorMenu)->getChild(Gui::EDM_LIST_LEVELS);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    tmpWin = getModeManager().getGui().getGuiSheet(Gui::editorMenu)->getChild(Gui::EDM_TEXT_LOADING);
    tmpWin->hide();
    mFilesList.clear();
    mDescriptionList.clear();
    levelSelectList->resetList();

    std::string levelPath = ResourceManager::getSingleton().getGameDataPath() + LEVEL_PATH_SKIRMISH;
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
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem("SK-" + mapName);
            item->setID(n);
            item->setSelectionBrushImage("OpenDungeonsSkin/SelectionBrush");
            levelSelectList->addItem(item);
        }
    }

    int skirmishSize = mFilesList.size();

    levelPath = ResourceManager::getSingleton().getGameDataPath() + LEVEL_PATH_MULTIPLAYER;
    if(Helper::fillFilesList(levelPath, mFilesList, LEVEL_EXTENSION))
    {
        for (uint32_t n = skirmishSize; n < mFilesList.size(); ++n)
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
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem("MP-" + mapName);
            item->setID(n);
            item->setSelectionBrushImage("OpenDungeonsSkin/SelectionBrush");
            levelSelectList->addItem(item);

            // We save the absolute path
            boost::filesystem::path p(mFilesList[n]);
            p = boost::filesystem::canonical(p);
            mFilesList[n] = p.string();
        }
    }
}

bool MenuModeEditor::launchSelectedButtonPressed(const CEGUI::EventArgs&)
{
    CEGUI::Window* tmpWin = getModeManager().getGui().getGuiSheet(Gui::editorMenu)->getChild(Gui::EDM_LIST_LEVELS);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    if(levelSelectList->getSelectedCount() == 0)
    {
        tmpWin = getModeManager().getGui().getGuiSheet(Gui::editorMenu)->getChild(Gui::EDM_TEXT_LOADING);
        tmpWin->setText("Please select a level first.");
        tmpWin->show();
        return true;
    }

    tmpWin = getModeManager().getGui().getGuiSheet(Gui::editorMenu)->getChild(Gui::EDM_TEXT_LOADING);
    tmpWin->setText("Loading...");
    tmpWin->show();

    CEGUI::ListboxItem* selItem = levelSelectList->getFirstSelectedItem();
    int id = selItem->getID();

    const std::string& level = mFilesList[id];

    // In single player mode, we act as a server
    if(!ODServer::getSingleton().startServer(level, ServerMode::ModeEditor))
    {
        LogManager::getSingleton().logMessage("ERROR: Could not start server for editor !!!");
        tmpWin = getModeManager().getGui().getGuiSheet(Gui::editorMenu)->getChild(Gui::EDM_TEXT_LOADING);
        tmpWin->setText("ERROR: Could not start server for editor !!!");
        tmpWin->show();
    }

    if(!ODClient::getSingleton().connect("localhost", ConfigManager::getSingleton().getNetworkPort()))
    {
        LogManager::getSingleton().logMessage("ERROR: Could not connect to server for editor !!!");
        tmpWin = getModeManager().getGui().getGuiSheet(Gui::editorMenu)->getChild(Gui::EDM_TEXT_LOADING);
        tmpWin->setText("Error: Couldn't connect to local server!");
        tmpWin->show();
        return true;
    }
    return true;
}

bool MenuModeEditor::updateDescription(const CEGUI::EventArgs&)
{
    // Get the level corresponding id
    CEGUI::Window* tmpWin = getModeManager().getGui().getGuiSheet(Gui::editorMenu)->getChild(Gui::SKM_LIST_LEVELS);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    CEGUI::Window* descTxt = getModeManager().getGui().getGuiSheet(Gui::editorMenu)->getChild("LevelWindowFrame/MapDescriptionText");

    if(levelSelectList->getSelectedCount() == 0)
    {
        descTxt->setText("");
        return true;
    }

    getModeManager().getGui().playButtonClickSound();

    CEGUI::ListboxItem* selItem = levelSelectList->getFirstSelectedItem();
    int id = selItem->getID();

    std::string description = mDescriptionList[id];
    descTxt->setText(description);
    return true;
}

bool MenuModeEditor::goBack(const CEGUI::EventArgs &)
{
    getModeManager().requestMode(AbstractModeManager::MAIN_MENU);
    return true;
}

