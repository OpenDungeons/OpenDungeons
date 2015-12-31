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
        MENU_MAIN = 1,
        MENU_SKIRMISH,
        MENU_MULTIPLAYER_CLIENT,
        MENU_MULTIPLAYER_SERVER,
        MENU_MASTERSERVER_JOIN,
        MENU_MASTERSERVER_HOST,
        MENU_EDITOR_NEW,
        MENU_EDITOR_LOAD,
        MENU_CONFIGURE_SEATS,
        MENU_REPLAY,
        MENU_LOAD_SAVEDGAME,
        GAME,
        EDITOR,
        NUM_ELEMS //Number of types
    };
    virtual ~AbstractModeManager()
    {}
    virtual ModeType getCurrentModeType() const = 0;
private:
    AbstractModeManager(const AbstractModeManager&) = delete;
    AbstractModeManager& operator=(const AbstractModeManager&) = delete;

};

#endif // ABSTRACTMODEMANAGER_H
