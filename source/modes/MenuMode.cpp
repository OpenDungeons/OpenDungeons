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
