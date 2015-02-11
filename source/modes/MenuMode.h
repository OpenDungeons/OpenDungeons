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

#ifndef MENUMODE_H
#define MENUMODE_H

#include "AbstractApplicationMode.h"

class MenuMode: public AbstractApplicationMode
{
public:
    MenuMode(ModeManager*);

    virtual ~MenuMode();

    //! \brief Called when the game mode is activated
    //! Used to call the corresponding Gui Sheet.
    void activate();
};

#endif // MENUMODE_H
