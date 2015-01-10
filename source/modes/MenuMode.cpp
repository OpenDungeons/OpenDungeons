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

#include "MenuMode.h"

#include "render/Gui.h"
#include "gamemap/GameMap.h"
#include "render/ODFrameListener.h"
#include "modes/ModeManager.h"
#include "sound/MusicPlayer.h"

MenuMode::MenuMode(ModeManager *modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU)
{
}

void MenuMode::activate()
{
    // Loads the corresponding Gui sheet.
    Gui::getSingleton().loadGuiSheet(Gui::mainMenu);

    giveFocus();

    // Play the main menu music
    // TODO: Make this configurable.
    MusicPlayer::getSingleton().play("Pal_Zoltan_Illes_OpenDungeons_maintheme.ogg");

    GameMap* gameMap = ODFrameListener::getSingletonPtr()->getClientGameMap();
    gameMap->clearAll();
    gameMap->setGamePaused(true);
}

MenuMode::~MenuMode()
{
}

bool MenuMode::mouseMoved(const OIS::MouseEvent &arg)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition((float)arg.state.X.abs, (float)arg.state.Y.abs);
}

bool MenuMode::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
        Gui::getSingletonPtr()->convertButton(id));
}

bool MenuMode::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
        Gui::getSingletonPtr()->convertButton(id));
}

bool MenuMode::keyPressed(const OIS::KeyEvent &arg) {
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

bool MenuMode::keyReleased(const OIS::KeyEvent &arg)
{
    return true;
}

void MenuMode::handleHotkeys(OIS::KeyCode keycode)
{
}
