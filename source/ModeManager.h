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

#ifndef MODEMANAGER_H
#define MODEMANAGER_H

#include <stack>

using std::stack;

class AbstractApplicationMode;
class ModeContext;
class GameMap;
class MiniMap;
class Console;
class CameraManager;

class ModeManager
{
    friend class Gui;

public:

    enum ModeType {
        MENU,
        GAME,
        EDITOR,
        CONSOLE,
        FPP,
        PREV,
    };

    ModeManager(GameMap* ,MiniMap*, Console*);
    ~ModeManager();

    AbstractApplicationMode* getCurrentMode();

    AbstractApplicationMode* progressMode(ModeType);
    AbstractApplicationMode* regressMode();

    int lookForNewMode();

    ModeContext *mMc;

private:
    Console *cn;
    AbstractApplicationMode* mModesArray[5];
    std::stack<ModeType> mModesStack;
};

#endif // MODEMANAGER_H
