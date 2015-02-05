/*!
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

#ifndef KEEPERAI_H
#define KEEPERAI_H

#include "ai/BaseAI.h"

class KeeperAI : public BaseAI
{

public:
    KeeperAI(GameMap& gameMap, Player& player, const std::string& parameters = std::string());
    virtual bool doTurn(double frameTime);

protected:
    //! \brief Checks if the AI has a treasury. If not, we search for the first available tile
    //! to add it
    //! Returns true if the action has been done and false if nothing has been done
    bool checkTreasury();

    //! \brief Checks if a room is needed. If yes, it will check if place is available
    //! and start digging for it.
    //! Returns true if the action has been done and false if nothing has been done
    bool handleRooms();

    //! \brief Look for gold and make way up to it.
    //! \brief Returns whether the action could succeed.
    //! It will also return false once it's done.
    bool lookForGold();

    //! \brief Picks up wounded creatures and drops then in the dungeon temple
    void saveWoundedCreatures();

    //! \brief Checks if we are under attack and does the needed if it is the case
    void handleDefense();

    //! \brief Checks if a new worker should be summoned
    //! Returns true if the action has been done and false if nothing has been done
    bool handleWorkers();

private:
    bool buildMostNeededRoom();

    int mCooldownCheckTreasury;
    int mCooldownLookingForRooms;
    int mRoomPosX;
    int mRoomPosY;
    int mRoomSize;
    bool mNoMoreReachableGold;
    int mCooldownLookingForGold;
    int mCooldownDefense;
    int mCooldownWorkers;
};

#endif // KEEPERAI_H
