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

#include "rooms/RoomManager.h"

#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "rooms/Room.h"
#include "rooms/RoomType.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

const std::string EMPTY_STRING;

int RoomFunctions::getRoomCostFunc(std::vector<Tile*>& targets, GameMap* gameMap, RoomType type,
    int tileX1, int tileY1, int tileX2, int tileY2, Player* player) const
{
    if(mGetRoomCostFunc == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "null mGetRoomCostFunc function Room=" + Helper::toString(static_cast<uint32_t>(type)));
        return 0;
    }

    return mGetRoomCostFunc(targets, gameMap, type, tileX1, tileY1, tileX2, tileY2, player);
}

void RoomFunctions::buildRoomFunc(GameMap* gameMap, RoomType type, const std::vector<Tile*>& targets,
    Seat* seat) const
{
    if(mBuildRoomFunc == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "null mBuildRoomFunc function Room=" + Helper::toString(static_cast<uint32_t>(type)));
        return;
    }

    mBuildRoomFunc(gameMap, targets, seat);
}

Room* RoomFunctions::getRoomFromStreamFunc(GameMap* gameMap, RoomType type, std::istream& is) const
{
    if(mGetRoomFromStreamFunc == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "null mGetRoomFromStreamFunc function Room=" + Helper::toString(static_cast<uint32_t>(type)));
        return nullptr;
    }

    return mGetRoomFromStreamFunc(gameMap, is);
}

std::vector<RoomFunctions>& getRoomFunctions()
{
    static std::vector<RoomFunctions> RoomList(static_cast<uint32_t>(RoomType::nbRooms));
    return RoomList;
}

int RoomManager::getRoomCost(std::vector<Tile*>& targets, GameMap* gameMap, RoomType type,
    int tileX1, int tileY1, int tileX2, int tileY2, Player* player)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getRoomFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return 0;
    }

    RoomFunctions& roomFuncs = getRoomFunctions()[index];
    return roomFuncs.getRoomCostFunc(targets, gameMap, type, tileX1, tileY1, tileX2, tileY2, player);
}

void RoomManager::buildRoom(GameMap* gameMap, RoomType type, const std::vector<Tile*>& targets,
    Seat* seat)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getRoomFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return;
    }

    RoomFunctions& roomFuncs = getRoomFunctions()[index];
    roomFuncs.buildRoomFunc(gameMap, type, targets, seat);
}

Room* RoomManager::getRoomFromStream(GameMap* gameMap, std::istream& is)
{
    RoomType type;
    OD_ASSERT_TRUE(is >> type);
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getRoomFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return nullptr;
    }

    RoomFunctions& roomFuncs = getRoomFunctions()[index];
    return roomFuncs.getRoomFromStreamFunc(gameMap, type, is);
}

const std::string& RoomManager::getRoomNameFromRoomType(RoomType type)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getRoomFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return EMPTY_STRING;
    }
    RoomFunctions& roomFuncs = getRoomFunctions()[index];
    return roomFuncs.mName;
}

RoomType RoomManager::getRoomTypeFromRoomName(const std::string& name)
{
    uint32_t nbRooms = static_cast<uint32_t>(RoomType::nbRooms);
    for(uint32_t i = 0; i < nbRooms; ++i)
    {
        RoomFunctions& roomFuncs = getRoomFunctions()[i];
        if(name.compare(roomFuncs.mName) == 0)
            return static_cast<RoomType>(i);
    }

    OD_ASSERT_TRUE_MSG(false, "Cannot find Room name=" + name);
    return RoomType::nullRoomType;
}

void RoomManager::registerRoom(RoomType type, const std::string& name,
    RoomFunctions::GetRoomCostFunc getRoomCostFunc,
    RoomFunctions::BuildRoomFunc buildRoomFunc,
    RoomFunctions::GetRoomFromStreamFunc getRoomFromStreamFunc)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getRoomFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return;
    }

    RoomFunctions& roomFuncs = getRoomFunctions()[index];
    roomFuncs.mName = name;
    roomFuncs.mGetRoomCostFunc = getRoomCostFunc;
    roomFuncs.mBuildRoomFunc = buildRoomFunc;
    roomFuncs.mGetRoomFromStreamFunc = getRoomFromStreamFunc;
}

int RoomManager::getRefundPrice(std::vector<Tile*>& tiles, GameMap* gameMap,
    int tileX1, int tileY1, int tileX2, int tileY2, Player* player)
{
    int price = 0;
    std::vector<Tile*> tilesRegion = gameMap->rectangularRegion(tileX1, tileY1, tileX2, tileY2);
    if(!gameMap->isServerGameMap())
    {
        // On client side, we don't fill tiles
        for(Tile* tile : tilesRegion)
            price += tile->getRefundPriceRoom();

        return price;
    }

    for(Tile* tile : tilesRegion)
    {
        Room* room = tile->getCoveringRoom();
        if(room == nullptr)
            continue;

        if(!room->canSeatSellBuilding(player->getSeat()))
            continue;

        tiles.push_back(tile);
        price += costPerTile(room->getType()) / 2;
    }
    return price;
}

void RoomManager::sellRoomTiles(GameMap* gameMap, const std::vector<Tile*>& tiles)
{
    std::set<Room*> rooms;
    for(Tile* tile : tiles)
    {
        Room* room = tile->getCoveringRoom();
        if(room == nullptr)
        {
            OD_ASSERT_TRUE_MSG(false, "tile=" + Tile::displayAsString(tile));
            continue;
        }
        OD_ASSERT_TRUE(room->removeCoveredTile(tile));
        rooms.insert(room);
    }

    // We notify the clients with vision of the changed tiles. Note that we need
    // to calculate per seat since the could have vision on different parts of the building
    std::map<Seat*,std::vector<Tile*>> tilesPerSeat;
    const std::vector<Seat*>& seats = gameMap->getSeats();
    for(Seat* seat : seats)
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        for(Tile* tile : tiles)
        {
            if(!seat->hasVisionOnTile(tile))
                continue;

            tile->changeNotifiedForSeat(seat);
            tilesPerSeat[seat].push_back(tile);
        }
    }

    for(const std::pair<Seat* const,std::vector<Tile*>>& p : tilesPerSeat)
    {
        uint32_t nbTiles = p.second.size();
        ServerNotification serverNotification(
            ServerNotificationType::refreshTiles, p.first->getPlayer());
        serverNotification.mPacket << nbTiles;
        for(Tile* tile : p.second)
        {
            gameMap->tileToPacket(serverNotification.mPacket, tile);
            p.first->updateTileStateForSeat(tile);
            p.first->exportTileToPacket(serverNotification.mPacket, tile);
        }
        ODServer::getSingleton().sendAsyncMsg(serverNotification);
    }

    // We update active spots of each impacted rooms
    for(Room* room : rooms)
        room->updateActiveSpots();
}

int RoomManager::costPerTile(RoomType t)
{
    switch (t)
    {
    case RoomType::nullRoomType:
        return 0;

    case RoomType::dungeonTemple:
        return 0;

    case RoomType::portal:
        return 0;

    case RoomType::treasury:
        return ConfigManager::getSingleton().getRoomConfigInt32("TreasuryCostPerTile");

    case RoomType::dormitory:
        return ConfigManager::getSingleton().getRoomConfigInt32("DormitoryCostPerTile");

    case RoomType::hatchery:
        return ConfigManager::getSingleton().getRoomConfigInt32("HatcheryCostPerTile");

    case RoomType::workshop:
        return ConfigManager::getSingleton().getRoomConfigInt32("WorkshopCostPerTile");

    case RoomType::trainingHall:
        return ConfigManager::getSingleton().getRoomConfigInt32("TrainHallCostPerTile");

    case RoomType::library:
        return ConfigManager::getSingleton().getRoomConfigInt32("LibraryCostPerTile");

    case RoomType::crypt:
        return ConfigManager::getSingleton().getRoomConfigInt32("CryptCostPerTile");

    default:
        return 0;
    }
}
