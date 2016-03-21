/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#include "modes/MenuModeMasterServerJoin.h"

#include "gamemap/GameMap.h"
#include "modes/ModeManager.h"
#include "network/ODServer.h"
#include "network/ODClient.h"
#include "render/Gui.h"
#include "render/ODFrameListener.h"
#include "sound/MusicPlayer.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/ResourceManager.h"
#include "ODApplication.h"

#include <CEGUI/CEGUI.h>

const Ogre::Real PERIOD_REFRESH_LIST = 10;

MenuModeMasterServerJoin::MenuModeMasterServerJoin(ModeManager *modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU_MASTERSERVER_JOIN),
    mTimeSinceLastUpdateList(0)
{
    CEGUI::Window* window = getModeManager().getGui().getGuiSheet(Gui::guiSheet::multiMasterServerJoinMenu);

    addEventConnection(
        window->getChild("LevelWindowFrame/ClientButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeMasterServerJoin::clientButtonPressed,
                                     this)
        )
    );
    addEventConnection(
        window->getChild("LevelWindowFrame/BackButton")->subscribeEvent(
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
        window->getChild("LevelWindowFrame/LevelSelect")->subscribeEvent(
            CEGUI::Listbox::EventMouseClick,
            CEGUI::Event::Subscriber(&MenuModeMasterServerJoin::updateDescription,
                                     this)
        )
    );

    Gui& gui = getModeManager().getGui();
    CEGUI::Window* mainWin = gui.getGuiSheet(Gui::guiSheet::multiMasterServerJoinMenu);
    CEGUI::MultiColumnList* levelSelectList = static_cast<CEGUI::MultiColumnList*>(
        mainWin->getChild("LevelWindowFrame/LevelSelect"));

    // We remove the columns from the list if any
    while(levelSelectList->getColumnCount() > 0)
        levelSelectList->removeColumn(0);

    levelSelectList->setSelectionMode(CEGUI::MultiColumnList::SelectionMode::RowSingle);
    levelSelectList->addColumn("Creator", 0, CEGUI::UDim(0.45f, 0));
    levelSelectList->addColumn("Level", 1, CEGUI::UDim(0.45f, 0));
}

void MenuModeMasterServerJoin::activate()
{
    // Loads the corresponding Gui sheet.
    Gui& gui = getModeManager().getGui();
    gui.loadGuiSheet(Gui::guiSheet::multiMasterServerJoinMenu);

    giveFocus();

    // Play the main menu music
    MusicPlayer::getSingleton().play(ConfigManager::getSingleton().getMainMenuMusic());

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->clearAll();
    gameMap->setGamePaused(true);

    CEGUI::Window* mainWin = gui.getGuiSheet(Gui::guiSheet::multiMasterServerJoinMenu);
    mainWin->getChild("LoadingText")->setText("");

    CEGUI::Editbox* editNick = static_cast<CEGUI::Editbox*>(
        mainWin->getChild("LevelWindowFrame/NickEdit"));
    ConfigManager& config = ConfigManager::getSingleton();
    std::string nickname = config.getGameValue(Config::NICKNAME, std::string(), false);
    if (!nickname.empty())
        editNick->setText(reinterpret_cast<const CEGUI::utf8*>(nickname.c_str()));

    refreshList();
}

void MenuModeMasterServerJoin::refreshList()
{
    Gui& gui = getModeManager().getGui();
    CEGUI::Window* mainWin = gui.getGuiSheet(Gui::guiSheet::multiMasterServerJoinMenu);
    CEGUI::MultiColumnList* levelSelectList = static_cast<CEGUI::MultiColumnList*>(
        mainWin->getChild("LevelWindowFrame/LevelSelect"));

    // If an item is already selected in the list, we save it so that it is selected
    // again once we refresh (if still available)
    std::string selUuid;
    if(levelSelectList->getSelectedCount() > 0)
    {
        CEGUI::ListboxItem* selItem = levelSelectList->getFirstSelectedItem();
        uint32_t id = selItem->getID();

        if(id >= mMasterServerGames.size())
        {
            OD_LOG_ERR("index too high=" + Helper::toString(id) + ", size=" + Helper::toString(mMasterServerGames.size()));
            return;
        }

        MasterServerGame& selGame = mMasterServerGames[id];
        selUuid = selGame.mUuid;
    }

    levelSelectList->resetList();
    mMasterServerGames.clear();

    if(MasterServer::fillMasterServerGames(ODApplication::VERSION, mMasterServerGames))
    {
        uint32_t id = 0;
        for(MasterServerGame& game : mMasterServerGames)
        {
            uint32_t rowIndex = levelSelectList->addRow(id);
            CEGUI::ListboxTextItem* item0 = new CEGUI::ListboxTextItem(game.mCreator);
            item0->setSelectionBrushImage("OpenDungeonsSkin/SelectionBrush");
            item0->setID(id);
            levelSelectList->setItem(item0, 0, rowIndex);
            CEGUI::ListboxTextItem* item1 = new CEGUI::ListboxTextItem(game.mLabel);
            item1->setSelectionBrushImage("OpenDungeonsSkin/SelectionBrush");
            item1->setID(id);
            levelSelectList->setItem(item1, 1, rowIndex);
            if(!selUuid.empty() && (selUuid.compare(game.mUuid) == 0))
            {
                levelSelectList->setItemSelectState(item0, true);
                levelSelectList->setItemSelectState(item1, true);
            }

            ++id;
        }
    }

    mTimeSinceLastUpdateList = 0;
}

void MenuModeMasterServerJoin::onFrameStarted(const Ogre::FrameEvent& evt)
{
    mTimeSinceLastUpdateList += evt.timeSinceLastFrame;
    if(mTimeSinceLastUpdateList >= PERIOD_REFRESH_LIST)
        refreshList();
}

bool MenuModeMasterServerJoin::clientButtonPressed(const CEGUI::EventArgs&)
{
    CEGUI::Window* mainWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::multiMasterServerJoinMenu);
    CEGUI::MultiColumnList* levelSelectList = static_cast<CEGUI::MultiColumnList*>(
        mainWin->getChild("LevelWindowFrame/LevelSelect"));
    CEGUI::Window* infoText = mainWin->getChild("LoadingText");

    if(levelSelectList->getSelectedCount() == 0)
    {
        infoText->setText("Please select a level first.");
        return true;
    }

    CEGUI::Editbox* editNick = static_cast<CEGUI::Editbox*>(
        mainWin->getChild("LevelWindowFrame/NickEdit"));
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

    CEGUI::ListboxItem* selItem = levelSelectList->getFirstSelectedItem();
    uint32_t id = selItem->getID();

    if(id >= mMasterServerGames.size())
    {
        OD_LOG_ERR("index too high=" + Helper::toString(id) + ", size=" + Helper::toString(mMasterServerGames.size()));
        return true;
    }

    MasterServerGame& selGame = mMasterServerGames[id];

    ODFrameListener::getSingleton().getClientGameMap()->setLocalPlayerNick(nick);

    const std::string& ip = selGame.mIp;
    const int32_t& port = selGame.mPort;
    uint32_t timeout = ConfigManager::getSingleton().getClientConnectionTimeout();
    std::string replayFilename = ResourceManager::getSingleton().getReplayDataPath()
        + ResourceManager::getSingleton().buildReplayFilename();
    if(!ODClient::getSingleton().connect(ip, port, timeout, replayFilename))
    {
        // Error while connecting
        infoText->setText("Could not connect to: " + ip + ", port=" + Helper::toString(port));
        return true;
    }

    infoText->setText("Loading...");
    return true;
}

bool MenuModeMasterServerJoin::updateDescription(const CEGUI::EventArgs&)
{
    // Get the level corresponding id
    CEGUI::Window* mainWin = getModeManager().getGui().getGuiSheet(Gui::multiMasterServerJoinMenu);
    CEGUI::MultiColumnList* levelSelectList = static_cast<CEGUI::MultiColumnList*>(
        mainWin->getChild("LevelWindowFrame/LevelSelect"));
    CEGUI::Window* descrTxt = mainWin->getChild("LevelWindowFrame/MapDescriptionText");

    if(levelSelectList->getSelectedCount() == 0)
    {
        descrTxt->setText("");
        return true;
    }

    getModeManager().getGui().playButtonClickSound();

    CEGUI::ListboxItem* selItem = levelSelectList->getFirstSelectedItem();
    uint32_t id = selItem->getID();
    if(id >= mMasterServerGames.size())
    {
        OD_LOG_ERR("index too high=" + Helper::toString(id) + ", size=" + Helper::toString(mMasterServerGames.size()));
        return true;
    }

    MasterServerGame& selGame = mMasterServerGames[id];
    descrTxt->setText(reinterpret_cast<const CEGUI::utf8*>(selGame.mDescr.c_str()));
    return true;
}
