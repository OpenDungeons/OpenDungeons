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

#ifndef MENUMODEEDITORNEW_H
#define MENUMODEEDITORNEW_H

#include "AbstractApplicationMode.h"

class MenuModeEditorNew: public AbstractApplicationMode
{
public:
    MenuModeEditorNew(ModeManager*);

    //! \brief Called when the game mode is activated
    //! Used to call the corresponding Gui Sheet.
    void activate() final override;

    bool launchSelectedButtonPressed(const CEGUI::EventArgs&);
};

#endif // MENUMODEEDITORNEW_H
