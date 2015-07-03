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

#include "modes/MenuModeSkirmish.h"

#include "utils/Helper.h"
#include "render/Gui.h"
#include "modes/ModeManager.h"
#include "sound/MusicPlayer.h"
#include "gamemap/GameMap.h"
#include "render/ODFrameListener.h"
#include "network/ODServer.h"
#include "network/ODClient.h"
#include "utils/LogManager.h"
#include "gamemap/MapLoader.h"
#include "utils/ConfigManager.h"
#include "utils/ResourceManager.h"

#include <CEGUI/CEGUI.h>
#include "boost/filesystem.hpp"

const std::string LEVEL_EXTENSION = ".level";

MenuModeSkirmish::MenuModeSkirmish(ModeManager* modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU_SKIRMISH)
{
    CEGUI::Window* window = modeManager->getGui().getGuiSheet(Gui::guiSheet::skirmishMenu);

    // Fills the Level type combo box with the available level types.
    const CEGUI::Image* selImg = &CEGUI::ImageManager::getSingleton().get("OpenDungeonsSkin/SelectionBrush");
    CEGUI::Combobox* levelTypeCb = static_cast<CEGUI::Combobox*>(window->getChild(Gui::SKM_LIST_LEVEL_TYPES));

    CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem("Skirmish Levels", 0);
    item->setSelectionBrushImage(selImg);
    levelTypeCb->addItem(item);

    item = new CEGUI::ListboxTextItem("Lan Solo", 1);
    item->setSelectionBrushImage(selImg);
    levelTypeCb->addItem(item);

    addEventConnection(
        window->getChild(Gui::SKM_BUTTON_LAUNCH)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeSkirmish::launchSelectedButtonPressed, this)
        )
    );
    addEventConnection(
        window->getChild(Gui::SKM_LIST_LEVELS)->subscribeEvent(
            CEGUI::Listbox::EventMouseDoubleClick,
            CEGUI::Event::Subscriber(&MenuModeSkirmish::launchSelectedButtonPressed, this)
        )
    );
    addEventConnection(
        window->getChild(Gui::SKM_LIST_LEVELS)->subscribeEvent(
            CEGUI::Listbox::EventMouseClick,
            CEGUI::Event::Subscriber(&MenuModeSkirmish::updateDescription, this)
        )
    );
    addEventConnection(
        window->getChild(Gui::SKM_BUTTON_BACK)->subscribeEvent(
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

    addEventConnection(
        window->getChild(Gui::SKM_LIST_LEVEL_TYPES)->subscribeEvent(
            CEGUI::Combobox::EventListSelectionAccepted,
            CEGUI::Event::Subscriber(&MenuModeSkirmish::updateFilesList, this)
        )
    );
}

void MenuModeSkirmish::activate()
{
    // Loads the corresponding Gui sheet.
    getModeManager().getGui().loadGuiSheet(Gui::guiSheet::skirmishMenu);

    giveFocus();

    // Play the main menu music
    // TODO: Make this configurable.
    MusicPlayer::getSingleton().play("OpenDungeonsMainTheme_pZi.ogg");

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->clearAll();
    gameMap->setGamePaused(true);

    // Select skirmish
    CEGUI::Combobox* levelTypeCb = static_cast<CEGUI::Combobox*>(getModeManager().getGui().
                                       getGuiSheet(Gui::skirmishMenu)->getChild(Gui::SKM_LIST_LEVEL_TYPES));
    levelTypeCb->setItemSelectState(static_cast<size_t>(0), true);
    updateFilesList();

    // Set the player name if valid. (Will use the defaut one if not.)
    ConfigManager& config = ConfigManager::getSingleton();
    std::string nickname = config.hasGameValue(Config::NICKNAME) ?
        config.getGameValue(Config::NICKNAME) : std::string();
    if (!nickname.empty())
        ODFrameListener::getSingleton().getClientGameMap()->setLocalPlayerNick(nickname);
}

bool MenuModeSkirmish::updateFilesList(const CEGUI::EventArgs&)
{
    CEGUI::Window* window = getModeManager().getGui().getGuiSheet(Gui::guiSheet::skirmishMenu);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(window->getChild(Gui::SKM_LIST_LEVELS));

    CEGUI::Combobox* levelTypeCb = static_cast<CEGUI::Combobox*>(window->getChild(Gui::SKM_LIST_LEVEL_TYPES));

    CEGUI::Window* loadText = window->getChild(Gui::SKM_TEXT_LOADING);
    loadText->setText("");
    mFilesList.clear();
    mDescriptionList.clear();
    levelSelectList->resetList();


    std::string levelPath;
    size_t selection = levelTypeCb->getItemIndex(levelTypeCb->getSelectedItem());
    switch (selection)
    {
        default:
        case 0:
            levelPath = ResourceManager::getSingleton().getLevelPathSkirmish();
            break;
        case 1:
            levelPath = ResourceManager::getSingleton().getLevelPathMultiplayer();
            break;
    }

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
        }
    }

    updateDescription();
    return true;
}

bool MenuModeSkirmish::launchSelectedButtonPressed(const CEGUI::EventArgs&)
{
    CEGUI::Window* mainWin = getModeManager().getGui().getGuiSheet(Gui::skirmishMenu);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(mainWin->getChild(Gui::SKM_LIST_LEVELS));

    if(levelSelectList->getSelectedCount() == 0)
    {
        mainWin->getChild(Gui::SKM_TEXT_LOADING)->setText("Please select a level first.");
        return true;
    }

    mainWin->getChild(Gui::SKM_TEXT_LOADING)->setText("Loading...");

    CEGUI::ListboxItem* selItem = levelSelectList->getFirstSelectedItem();
    int id = selItem->getID();

    const std::string& level = mFilesList[id];
    // In single player mode, we act as a server
    if(!ODServer::getSingleton().startServer(level, ServerMode::ModeGameSinglePlayer))
    {
        OD_LOG_ERR("Could not start server for single player game !!!");
        mainWin->getChild(Gui::SKM_TEXT_LOADING)->setText("ERROR: Could not start server for single player game !!!");
        return true;
    }

    if(!ODClient::getSingleton().connect("localhost", ConfigManager::getSingleton().getNetworkPort()))
    {
        OD_LOG_ERR("Could not connect to server for single player game !!!");
        mainWin->getChild(Gui::SKM_TEXT_LOADING)->setText("Error: Couldn't connect to local server!");
        return true;
    }
    return true;
}

bool MenuModeSkirmish::updateDescription(const CEGUI::EventArgs&)
{
    // Get the level corresponding id
    CEGUI::Window* mainWin = getModeManager().getGui().getGuiSheet(Gui::skirmishMenu);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(mainWin->getChild(Gui::SKM_LIST_LEVELS));

    CEGUI::Window* descTxt = mainWin->getChild("LevelWindowFrame/MapDescriptionText");

    if(levelSelectList->getSelectedCount() == 0)
    {
        descTxt->setText("");
        return true;
    }

    getModeManager().getGui().playButtonClickSound();

    CEGUI::ListboxItem* selItem = levelSelectList->getFirstSelectedItem();
    int id = selItem->getID();

    std::string description = mDescriptionList[id];
    descTxt->setText(reinterpret_cast<const CEGUI::utf8*>(description.c_str()));

    return true;
}
