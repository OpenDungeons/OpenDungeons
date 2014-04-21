/*!
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

#ifndef KEEPERAI_H
#define KEEPERAI_H

#include "BaseAI.h"
#include "AIFactory.h"

class KeeperAI : public BaseAI
{

public:
    KeeperAI(GameMap& gameMap, Player& player, const std::string& parameters = std::string());
    virtual bool doTurn(double frameTime);

protected:
    //! \brief Create some room around the temple.
    void MakeSomePlace();

    //! \brief Create a sleep room once we've got enough gold
    void buildSleepRoom();

    //! \brief Look for gold and make way up to it.
    void lookForGold();

private:
    static AIFactoryRegister<KeeperAI> reg;

    bool mSomePlaceMade;
    bool mSleepRoomMade;
    double mLastTimeLookingForGold;
};

#endif // KEEPERAI_H
