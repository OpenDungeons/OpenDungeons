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

#ifndef MENUMODECONFIGURESEATS_H
#define MENUMODECONFIGURESEATS_H

#include "AbstractApplicationMode.h"

#include <cstdint>

class Seat;
class Player;
class ODPacket;

namespace CEGUI
{
    class EventArgs;
}

class MenuModeConfigureSeats: public AbstractApplicationMode
{
public:
    MenuModeConfigureSeats(ModeManager*);

    virtual ~MenuModeConfigureSeats();

    //! \brief Called when the game mode is activated
    //! Used to call the corresponding Gui Sheet.
    void activate();

    bool launchSelectedButtonPressed(const CEGUI::EventArgs&);
    bool goBack(const CEGUI::EventArgs& e = {});

    bool comboChanged(const CEGUI::EventArgs& ea);
    void addPlayer(const std::string& nick, int32_t id);
    void removePlayer(int32_t id);

    void activatePlayerConfig();
    void refreshSeatConfiguration(ODPacket& packet);

private:
    bool mIsActivePlayerConfig;
    std::vector<int> mSeatIds;
    std::vector<std::pair<std::string, int32_t> > mPlayers;

    void fireSeatConfigurationToServer();
};

#endif // MENUMODECONFIGURESEATS_H
