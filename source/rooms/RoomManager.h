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

#ifndef ROOMMANAGER_H
#define ROOMMANAGER_H

#include <vector>
#include <istream>
#include <cstdint>

class GameMap;
class Player;
class Room;
class Seat;
class Tile;

enum class RoomType;

//! Class to gather functions used for rooms
class RoomFunctions
{
    friend class RoomManager;
public:
    typedef int (*GetRoomCostFunc)(std::vector<Tile*>& targets, GameMap* gameMap,
        RoomType type, int tileX1, int tileY1, int tileX2, int tileY2, Player* player);
    typedef void (*BuildRoomFunc)(GameMap*, const std::vector<Tile*>&, Seat*);
    typedef Room* (*GetRoomFromStreamFunc)(GameMap* gameMap, std::istream& is);

    RoomFunctions() :
        mGetRoomCostFunc(nullptr),
        mBuildRoomFunc(nullptr),
        mGetRoomFromStreamFunc(nullptr)
    {}

    int getRoomCostFunc(std::vector<Tile*>& targets, GameMap* gameMap, RoomType type,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player) const;

    void buildRoomFunc(GameMap* gameMap, RoomType type, const std::vector<Tile*>& targets,
        Seat* seat) const;

    Room* getRoomFromStreamFunc(GameMap* gameMap, RoomType type, std::istream& is) const;

private:
    std::string mName;
    GetRoomCostFunc mGetRoomCostFunc;
    BuildRoomFunc mBuildRoomFunc;
    GetRoomFromStreamFunc mGetRoomFromStreamFunc;

};

class RoomManager
{
public:
    //! Returns the Room cost required to build the room for the given player. targets will
    //! be filled with the suitable tiles. Note that if there are more targets than available gold,
    //! most rooms will fail to build.
    //! If no target is available, this function should return the gold needed for 1 target and
    //! targets vector should be empty
    static int getRoomCost(std::vector<Tile*>& targets, GameMap* gameMap, RoomType type,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player);

    //! Builds the Room. In most of the cases, targets should be the vector filled by getRoomCost
    static void buildRoom(GameMap* gameMap, RoomType type, const std::vector<Tile*>& targets,
        Seat* seat);

    /*! \brief Exports the headers needed to recreate the Room. It allows to extend Rooms as much as wanted.
     * The content of the Room will be exported by exportToStream.
     */
    static Room* getRoomFromStream(GameMap* gameMap, std::istream &is);

    static const std::string& getRoomNameFromRoomType(RoomType type);

    static RoomType getRoomTypeFromRoomName(const std::string& name);

    static int getRefundPrice(std::vector<Tile*>& tiles, GameMap* gameMap,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player);

    static void sellRoomTiles(GameMap* gameMap, const std::vector<Tile*>& tiles);

    static int costPerTile(RoomType t);

private:
    static void registerRoom(RoomType type, const std::string& name,
        RoomFunctions::GetRoomCostFunc getRoomCostFunc,
        RoomFunctions::BuildRoomFunc buildRoomFunc,
        RoomFunctions::GetRoomFromStreamFunc getRoomFromStreamFunc);

    template <typename D>
    static int getRoomCostReg(std::vector<Tile*>& targets, GameMap* gameMap, RoomType type,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player)
    {
        return D::getRoomCost(targets, gameMap, type, tileX1, tileY1, tileX2, tileY2, player);
    }

    template <typename D>
    static void buildRoomReg(GameMap* gameMap, const std::vector<Tile*>& targets, Seat* seat)
    {
        D::buildRoom(gameMap, targets, seat);
    }

    template <typename D>
    static Room* getRoomFromStreamReg(GameMap* gameMap, std::istream& is)
    {
        return D::getRoomFromStream(gameMap, is);
    }

    template <typename T> friend class RoomManagerRegister;
};

template <typename T>
class RoomManagerRegister
{
public:
    RoomManagerRegister(RoomType RoomType, const std::string& name)
    {
        RoomManager::registerRoom(RoomType, name, &RoomManager::getRoomCostReg<T>,
            &RoomManager::buildRoomReg<T>, &RoomManager::getRoomFromStreamReg<T>);
    }

private:
    RoomManagerRegister(const std::string& name, const RoomManagerRegister&);
};


#endif // ROOMMANAGER_H
