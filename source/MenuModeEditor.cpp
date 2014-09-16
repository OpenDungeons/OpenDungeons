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

#include "MenuModeEditor.h"

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

const std::string LEVEL_PATH_SKIRMISH = "./levels/skirmish/";
const std::string LEVEL_PATH_MULTIPLAYER = "./levels/multiplayer/";
const std::string LEVEL_EXTENSION = ".level";

MenuModeEditor::MenuModeEditor(ModeManager *modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU_EDITOR),
    mReadyToStartMode(false)
{
}

MenuModeEditor::~MenuModeEditor()
{
}

void MenuModeEditor::activate()
{
    // Loads the corresponding Gui sheet.
    Gui::getSingleton().loadGuiSheet(Gui::editorMenu);

    giveFocus();

    // Play the main menu music
    // TODO: Make this configurable.
    MusicPlayer::getSingleton().play("Pal_Zoltan_Illes_OpenDungeons_maintheme.ogg");

    ODFrameListener::getSingleton().getClientGameMap()->setGamePaused(true);

    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::editorMenu)->getChild(Gui::EDM_LIST_LEVELS);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    tmpWin = Gui::getSingleton().getGuiSheet(Gui::editorMenu)->getChild(Gui::EDM_TEXT_LOADING);
    tmpWin->hide();
    mListFilesSkirmish.clear();
    mListFilesMultiplayer.clear();
    levelSelectList->resetList();

    if(Helper::fillFileStemsList(LEVEL_PATH_SKIRMISH, mListFilesSkirmish, LEVEL_EXTENSION))
    {
        for (int n = 0; n < mListFilesSkirmish.size(); ++n)
        {
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem("SKIRMISH - " + mListFilesSkirmish[n]);
            item->setID(n);
            item->setSelectionBrushImage("OpenDungeonsOldSkin/ListboxSelectionBrush");
            levelSelectList->addItem(item);
        }
    }

    if(Helper::fillFileStemsList(LEVEL_PATH_MULTIPLAYER, mListFilesMultiplayer, LEVEL_EXTENSION))
    {
        for (int n = 0; n < mListFilesMultiplayer.size(); ++n)
        {
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem("MULTIPLAYER - " + mListFilesMultiplayer[n]);
            item->setID(n + mListFilesSkirmish.size());
            item->setSelectionBrushImage("OpenDungeonsOldSkin/ListboxSelectionBrush");
            levelSelectList->addItem(item);
        }
    }
}

void MenuModeEditor::launchSelectedButtonPressed()
{
    CEGUI::Window* tmpWin = Gui::getSingleton().getGuiSheet(Gui::editorMenu)->getChild(Gui::EDM_LIST_LEVELS);
    CEGUI::Listbox* levelSelectList = static_cast<CEGUI::Listbox*>(tmpWin);

    if(levelSelectList->getSelectedCount() == 0)
    {
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::editorMenu)->getChild(Gui::EDM_TEXT_LOADING);
        tmpWin->setText("Please select a level first.");
        tmpWin->show();
        return;
    }

    tmpWin = Gui::getSingleton().getGuiSheet(Gui::editorMenu)->getChild(Gui::EDM_TEXT_LOADING);
    tmpWin->setText("Loading...");
    tmpWin->show();

    CEGUI::ListboxItem* selItem = levelSelectList->getFirstSelectedItem();
    int id = selItem->getID();

    std::string level;
    if(id < mListFilesSkirmish.size())
        level = LEVEL_PATH_SKIRMISH + mListFilesSkirmish[id] + LEVEL_EXTENSION;
    else
        level = LEVEL_PATH_MULTIPLAYER + mListFilesMultiplayer[id - mListFilesSkirmish.size()] + LEVEL_EXTENSION;
    // In single player mode, we act as a server
    if(!ODServer::getSingleton().startServer(level, true, ODServer::ServerMode::ModeEditor))
    {
        LogManager::getSingleton().logMessage("ERROR: Could not start server for editor !!!");
    }

    if(!ODClient::getSingleton().connect("", ODApplication::PORT_NUMBER))
    {
        LogManager::getSingleton().logMessage("ERROR: Could not connect to server for editor !!!");
        tmpWin = Gui::getSingleton().getGuiSheet(Gui::editorMenu)->getChild(Gui::EDM_TEXT_LOADING);
        tmpWin->setText("Error: Couldn't connect to local server!");
        tmpWin->show();
        return;
    }

    // Makes the frame listener process client and server messages.
    mReadyToStartMode = true;
}

void MenuModeEditor::listLevelsClicked()
{
    launchSelectedButtonPressed();
}

bool MenuModeEditor::mouseMoved(const OIS::MouseEvent &arg)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition((float)arg.state.X.abs, (float)arg.state.Y.abs);
}

bool MenuModeEditor::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
        Gui::getSingletonPtr()->convertButton(id));
}

bool MenuModeEditor::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
        Gui::getSingletonPtr()->convertButton(id));
}

bool MenuModeEditor::keyPressed(const OIS::KeyEvent &arg)
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

bool MenuModeEditor::keyReleased(const OIS::KeyEvent &arg)
{
    return true;
}

void MenuModeEditor::handleHotkeys(OIS::KeyCode keycode)
{
}
