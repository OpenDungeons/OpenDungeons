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

#ifndef MENUMODEMASTERSERVERJOIN_H
#define MENUMODEMASTERSERVERJOIN_H

#include "AbstractApplicationMode.h"
#include "utils/MasterServer.h"

class MenuModeMasterServerJoin: public AbstractApplicationMode
{
public:
    MenuModeMasterServerJoin(ModeManager*);

    //! \brief Called when the game mode is activated
    //! Used to call the corresponding Gui Sheet.
    void activate() final override;

    bool clientButtonPressed(const CEGUI::EventArgs&);
    bool updateDescription(const CEGUI::EventArgs&);

    void onFrameStarted(const Ogre::FrameEvent& evt) override;

private:
    void refreshList();

    std::vector<MasterServerGame> mMasterServerGames;
    Ogre::Real mTimeSinceLastUpdateList;
};

#endif // MENUMODEMASTERSERVERJOIN_H
