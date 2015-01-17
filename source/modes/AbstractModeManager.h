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

#ifndef ABSTRACTMODEMANAGER_H
#define ABSTRACTMODEMANAGER_H

class AbstractModeManager
{
public:
    AbstractModeManager()
    {};
    //Enum for describing mode type
    enum ModeType
    {
        NONE = 0, // No change requested
        MENU = 1,
        MENU_SKIRMISH,
        MENU_MULTIPLAYER_CLIENT,
        MENU_MULTIPLAYER_SERVER,
        MENU_EDITOR,
        MENU_CONFIGURE_SEATS,
        MENU_REPLAY,
        GAME,
        EDITOR,
        CONSOLE,
        FPP,
        PREV, // Parent game mode requested
        ALL, //Used for console commands working in all modes
        NUM_ELEMS //Number of types
    };
    virtual ~AbstractModeManager()
    {}
    virtual ModeType getCurrentModeTypeExceptConsole() const = 0;
private:
    AbstractModeManager(const AbstractModeManager&) = delete;
    AbstractModeManager& operator=(const AbstractModeManager&) = delete;

};

#endif // ABSTRACTMODEMANAGER_H
