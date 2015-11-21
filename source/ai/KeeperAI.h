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

enum class RoomType;

class KeeperAI : public BaseAI
{

public:
    KeeperAI(GameMap& gameMap, Player& player, const std::string& parameters = std::string());
    virtual bool doTurn(double timeSinceLastTurn);

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

    //! \brief Checks if a room needs to be repaired and repairs if so
    //! Returns true if the action has been done and false if nothing has been done
    bool repairRooms();

    //! \brief Try to help tired creatures to reach dormitory
    //! Returns true if the action has been done and false if nothing has been done
    bool handleTiredCreatures();

    //! \brief Try to help hungry creatures to reach hatchery
    //! Returns true if the action has been done and false if nothing has been done
    bool handleHungryCreatures();

    //! \brief This function will be called once only during this player's first upkeep
    //! It should setup what is needed for the AI
    void handleFirstTurn();

private:
    //! \brief try to build the most needed available room
    bool buildMostNeededRoom();

    //! \brief Try to build the given room on the given position. Returns true if the room was built and
    //! false otherwise
    bool buildRoom(RoomType roomType, int x, int y, int roomSize);

    //! \brief Returns true if the given room is needed and false otherwise
    bool checkNeedRoom(RoomType roomType);

    int mCooldownCheckTreasury;
    int mCooldownLookingForRooms;
    int mRoomPosX;
    int mRoomPosY;
    int mRoomSize;
    bool mNoMoreReachableGold;
    int mCooldownLookingForGold;
    int mCooldownDefense;
    int mCooldownWorkers;
    int mCooldownRepairRooms;
    bool mIsFirstUpkeepDone;
};

#endif // KEEPERAI_H
