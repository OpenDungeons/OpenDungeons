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

#include <CEGUI/CEGUI.h>
#include "boost/filesystem.hpp"

const std::string LEVEL_PATH = "./levels/skirmish/";
const std::string LEVEL_EXTENSION = ".level";

MenuModeSkirmish::MenuModeSkirmish(ModeManager *modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU_SKIRMISH)
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

void MenuModeSkirmish::launchSelectedButtonPressed()
{
    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::skirmishMenu)->getChild(Gui::SKM_LIST_LEVELS);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    if(levelSelectList->getSelectedCount() > 0)
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::skirmishMenu)->getChild(Gui::SKM_TEXT_LOADING);
        tmpWin->show();

        CEGUI::ListboxItem*	selItem = levelSelectList->getFirstSelectedItem();
        int id = selItem->getID();

        std::string level = LEVEL_PATH + mListFiles[id] + LEVEL_EXTENSION;
        // In single player mode, we act as a server
        if(!ODServer::getSingleton().startServer(level, true))
        {
            LogManager::getSingleton().logMessage("ERROR: Could not start server for single player game !!!");
        }

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

void MenuModeSkirmish::listLevelsClicked()
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
