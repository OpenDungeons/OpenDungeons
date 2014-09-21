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

#include "MenuModeSkirmish.h"

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
#include "MapLoader.h"

#include <CEGUI/CEGUI.h>
#include "boost/filesystem.hpp"

const std::string LEVEL_PATH = "./levels/skirmish/";
const std::string LEVEL_EXTENSION = ".level";

MenuModeSkirmish::MenuModeSkirmish(ModeManager *modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU_SKIRMISH),
    mReadyToStartGame(false)
{
}

MenuModeSkirmish::~MenuModeSkirmish()
{
}

void MenuModeSkirmish::activate()
{
    // Loads the corresponding Gui sheet.
    Gui::getSingleton().loadGuiSheet(Gui::skirmishMenu);

    giveFocus();

    // Play the main menu music
    // TODO: Make this configurable.
    MusicPlayer::getSingleton().play("Pal_Zoltan_Illes_OpenDungeons_maintheme.ogg");

    ODFrameListener::getSingleton().getClientGameMap()->setGamePaused(true);

    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::skirmishMenu)->getChild(Gui::SKM_LIST_LEVELS);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    tmpWin = Gui::getSingleton().getGuiSheet(Gui::skirmishMenu)->getChild(Gui::SKM_TEXT_LOADING);
    tmpWin->hide();
    mFilesList.clear();
    mDescriptionList.clear();
    levelSelectList->resetList();

    if(Helper::fillFilesList(LEVEL_PATH, mFilesList, LEVEL_EXTENSION))
    {
        for (uint32_t n = 0; n < mFilesList.size(); ++n)
        {
            std::string filename = mFilesList[n];
            std::string mapName = MapLoader::getMapName(filename);
            std::string mapDescription = MapLoader::getMapDescription(filename);
            mDescriptionList.push_back(mapDescription);
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(mapName);
            item->setID(n);
            item->setSelectionBrushImage("OpenDungeonsSkin/SelectionBrush");
            levelSelectList->addItem(item);
        }
    }
}

void MenuModeSkirmish::launchSelectedButtonPressed()
{
    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::skirmishMenu)->getChild(Gui::SKM_LIST_LEVELS);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    if(levelSelectList->getSelectedCount() == 0)
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::skirmishMenu)->getChild(Gui::SKM_TEXT_LOADING);
        tmpWin->setText("Please select a level first.");
        tmpWin->show();
        return;
    }

    tmpWin = Gui::getSingleton().getGuiSheet(Gui::skirmishMenu)->getChild(Gui::SKM_TEXT_LOADING);
    tmpWin->setText("Loading...");
    tmpWin->show();

    CEGUI::ListboxItem* selItem = levelSelectList->getFirstSelectedItem();
    int id = selItem->getID();

    std::string level = mFilesList[id];
    // In single player mode, we act as a server
    if(!ODServer::getSingleton().startServer(level, true, ODServer::ServerMode::ModeGame))
    {
        LogManager::getSingleton().logMessage("ERROR: Could not start server for single player game !!!");
    }

    if(!ODClient::getSingleton().connect("", ODApplication::PORT_NUMBER))
    {
        LogManager::getSingleton().logMessage("ERROR: Could not connect to server for single player game !!!");
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::skirmishMenu)->getChild(Gui::SKM_TEXT_LOADING);
        tmpWin->setText("Error: Couldn't connect to local server!");
        tmpWin->show();
        return;
    }

    // Makes the frame listener process client and server messages.
    mReadyToStartGame = true;
}

void MenuModeSkirmish::updateDescription()
{
    // Get the level corresponding id
    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::skirmishMenu)->getChild(Gui::SKM_LIST_LEVELS);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    CEGUI::Window* descTxt = Gui::getSingleton().getGuiSheet(Gui::skirmishMenu)->getChild("LevelWindowFrame/MapDescriptionText");

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

void MenuModeSkirmish::listLevelsClicked()
{
    updateDescription();
}

void MenuModeSkirmish::listLevelsDoubleClicked()
{
    launchSelectedButtonPressed();
}

bool MenuModeSkirmish::mouseMoved(const OIS::MouseEvent &arg)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition((float)arg.state.X.abs, (float)arg.state.Y.abs);
}

bool MenuModeSkirmish::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
        Gui::getSingletonPtr()->convertButton(id));
}

bool MenuModeSkirmish::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
        Gui::getSingletonPtr()->convertButton(id));
}

bool MenuModeSkirmish::keyPressed(const OIS::KeyEvent &arg)
{
    switch (arg.key)
    {

    case OIS::KC_ESCAPE:
        regressMode();
        break;
    default:
        break;
    }
    return true;
}

bool MenuModeSkirmish::keyReleased(const OIS::KeyEvent &arg)
{
    return true;
}

void MenuModeSkirmish::handleHotkeys(OIS::KeyCode keycode)
{
}
