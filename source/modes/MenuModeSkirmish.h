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

#ifndef MENUMODESKIRMISH_H
#define MENUMODESKIRMISH_H

#include "AbstractApplicationMode.h"

class MenuModeSkirmish: public AbstractApplicationMode
{
public:
    MenuModeSkirmish(ModeManager*);

    virtual ~MenuModeSkirmish();

    //! \brief Called when the game mode is activated
    //! Used to call the corresponding Gui Sheet.
    void activate();

    void launchSelectedButtonPressed();

    void updateDescription();

    void listLevelsClicked();
    void listLevelsDoubleClicked();

private:
    std::vector<std::string> mFilesList;
    std::vector<std::string> mDescriptionList;
};

#endif // MENUMODESKIRMISH_H
