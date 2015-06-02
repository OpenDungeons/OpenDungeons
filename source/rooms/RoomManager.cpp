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
#include "modes/InputCommand.h"
#include "modes/InputManager.h"
#include "network/ClientNotification.h"
#include "network/ODClient.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "rooms/Room.h"
#include "rooms/RoomType.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

const std::string EMPTY_STRING;

namespace
{
    std::vector<RoomFunctions>& getRoomFunctions()
    {
        static std::vector<RoomFunctions> roomList(static_cast<uint32_t>(RoomType::nbRooms));
        return roomList;
    }
}

void RoomFunctions::checkBuildRoomFunc(GameMap* gameMap, RoomType type, const InputManager& inputManager, InputCommand& inputCommand) const
{
    if(mCheckBuildRoomFunc == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "null mCheckBuildRoom function Room=" + Helper::toString(static_cast<uint32_t>(type)));
        return;
    }

    mCheckBuildRoomFunc(gameMap, inputManager, inputCommand);
}

bool RoomFunctions::buildRoomFunc(GameMap* gameMap, RoomType type, Player* player, ODPacket& packet) const
{
    if(mBuildRoomFunc == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "null mBuildRoomFunc function Room=" + Helper::toString(static_cast<uint32_t>(type)));
        return false;
    }

    return mBuildRoomFunc(gameMap, player, packet);
}

void RoomFunctions::checkBuildRoomEditorFunc(GameMap* gameMap, RoomType type, const InputManager& inputManager, InputCommand& inputCommand) const
{
    if(mCheckBuildRoomEditorFunc == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "null mCheckBuildRoomEditorFund function Room=" + Helper::toString(static_cast<uint32_t>(type)));
        return;
    }

    mCheckBuildRoomEditorFunc(gameMap, inputManager, inputCommand);
}

bool RoomFunctions::buildRoomEditorFunc(GameMap* gameMap, RoomType type, ODPacket& packet) const
{
    if(mBuildRoomEditorFunc == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "null mBuildRoomEditorFunc function Room=" + Helper::toString(static_cast<uint32_t>(type)));
        return false;
    }

    return mBuildRoomEditorFunc(gameMap, packet);
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

void RoomManager::checkBuildRoom(GameMap* gameMap, RoomType type, const InputManager& inputManager, InputCommand& inputCommand)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getRoomFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return;
    }

    RoomFunctions& roomFuncs = getRoomFunctions()[index];
    roomFuncs.checkBuildRoomFunc(gameMap, type, inputManager, inputCommand);
}

bool RoomManager::buildRoom(GameMap* gameMap, RoomType type, Player* player, ODPacket& packet)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getRoomFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return false;
    }

    RoomFunctions& roomFuncs = getRoomFunctions()[index];
    return roomFuncs.buildRoomFunc(gameMap, type, player, packet);
}

void RoomManager::checkBuildRoomEditor(GameMap* gameMap, RoomType type, const InputManager& inputManager, InputCommand& inputCommand)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getRoomFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return;
    }

    RoomFunctions& roomFuncs = getRoomFunctions()[index];
    roomFuncs.checkBuildRoomEditorFunc(gameMap, type, inputManager, inputCommand);
}

bool RoomManager::buildRoomEditor(GameMap* gameMap, RoomType type, ODPacket& packet)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getRoomFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return false;
    }

    RoomFunctions& roomFuncs = getRoomFunctions()[index];
    return roomFuncs.buildRoomEditorFunc(gameMap, type, packet);
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
    RoomFunctions::CheckBuildRoomFunc checkBuildRoomFunc,
    RoomFunctions::BuildRoomFunc buildRoomFunc,
    RoomFunctions::CheckBuildRoomFunc checkBuildRoomEditorFunc,
    RoomFunctions::BuildRoomEditorFunc buildRoomEditorFunc,
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
    roomFuncs.mCheckBuildRoomFunc = checkBuildRoomFunc;
    roomFuncs.mBuildRoomFunc = buildRoomFunc;
    roomFuncs.mCheckBuildRoomEditorFunc = checkBuildRoomEditorFunc;
    roomFuncs.mBuildRoomEditorFunc = buildRoomEditorFunc;
    roomFuncs.mGetRoomFromStreamFunc = getRoomFromStreamFunc;
}

void RoomManager::checkSellRoomTiles(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    Player* player = gameMap->getLocalPlayer();
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        // We do not differentiate between room and trap (because there is no way to know on client side).
        // Note that price = 0 doesn't mean that the building is not a room
        Tile* tile = gameMap->getTile(inputManager.mXPos, inputManager.mYPos);
        if((tile == nullptr) || (!tile->getIsBuilding()) || (tile->getSeat() != player->getSeat()))
        {
            std::string txt = formatSellRoom(0);
            inputCommand.displayText(Ogre::ColourValue::White, txt);
            inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos, inputManager.mYPos);
            return;
        }

        uint32_t price = tile->getRefundPriceRoom();
        std::string txt = formatSellRoom(price);
        inputCommand.displayText(Ogre::ColourValue::White, txt);
        std::vector<Tile*> tiles;
        tiles.push_back(tile);
        inputCommand.selectTiles(tiles);
        return;
    }

    std::vector<Tile*> sellTiles;
    std::vector<Tile*> tiles = gameMap->rectangularRegion(inputManager.mXPos,
        inputManager.mYPos, inputManager.mLStartDragX, inputManager.mLStartDragY);
    uint32_t priceTotal = 0;
    for(Tile* tile : tiles)
    {
        if(!tile->getIsBuilding())
            continue;

        if(tile->getSeat() != player->getSeat())
            continue;

        sellTiles.push_back(tile);
        priceTotal += tile->getRefundPriceRoom();
    }

    if(inputManager.mCommandState == InputCommandState::building)
    {
        inputCommand.selectTiles(sellTiles);
        std::string txt = formatSellRoom(priceTotal);
        inputCommand.displayText(Ogre::ColourValue::White, txt);
        return;
    }

    inputCommand.unselectAllTiles();

    ClientNotification *clientNotification = new ClientNotification(
        ClientNotificationType::askSellRoomTiles);
    uint32_t nbTiles = sellTiles.size();
    clientNotification->mPacket << nbTiles;
    for(Tile* tile : sellTiles)
        gameMap->tileToPacket(clientNotification->mPacket, tile);

    ODClient::getSingleton().queueClientNotification(clientNotification);
}

void RoomManager::sellRoomTiles(GameMap* gameMap, Player* player, ODPacket& packet)
{
    uint32_t nbTiles;
    OD_ASSERT_TRUE(packet >> nbTiles);
    int32_t price = 0;
    std::set<Room*> rooms;
    std::vector<Tile*> tiles;
    while(nbTiles > 0)
    {
        --nbTiles;
        Tile* tile = gameMap->tileFromPacket(packet);
        if(tile == nullptr)
        {
            OD_ASSERT_TRUE_MSG(false, "tile=" + Tile::displayAsString(tile));
            continue;
        }
        Room* room = tile->getCoveringRoom();
        if(room == nullptr)
            continue;

        if(!room->canSeatSellBuilding(player->getSeat()))
            continue;

        if(!room->removeCoveredTile(tile))
        {
            OD_ASSERT_TRUE_MSG(false, "room=" + room->getName() + ", tile=" + Tile::displayAsString(tile) + ", seatId=" + Helper::toString(player->getSeat()->getId()));
            continue;
        }

        price += costPerTile(room->getType()) / 2;
        tiles.push_back(tile);
        rooms.insert(room);
    }

    gameMap->addGoldToSeat(price, player->getSeat()->getId());

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

std::string RoomManager::formatSellRoom(int price)
{
    return "retrieve " + Helper::toString(price) + " gold";
}

void RoomManager::checkSellRoomTilesEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        // We do not differentiate between room and trap (because there is no way to know on client side).
        // Note that price = 0 doesn't mean that the building is not a room
        Tile* tile = gameMap->getTile(inputManager.mXPos, inputManager.mYPos);
        if((tile == nullptr) || (!tile->getIsBuilding()))
        {
            inputCommand.displayText(Ogre::ColourValue::White, "Remove tiles");
            inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos, inputManager.mYPos);
            return;
        }

        inputCommand.displayText(Ogre::ColourValue::White, "Remove tiles");
        std::vector<Tile*> tiles;
        tiles.push_back(tile);
        inputCommand.selectTiles(tiles);
        return;
    }

    std::vector<Tile*> sellTiles;
    std::vector<Tile*> tiles = gameMap->rectangularRegion(inputManager.mXPos,
        inputManager.mYPos, inputManager.mLStartDragX, inputManager.mLStartDragY);
    for(Tile* tile : tiles)
    {
        if(!tile->getIsBuilding())
            continue;

        sellTiles.push_back(tile);
    }

    if(inputManager.mCommandState == InputCommandState::building)
    {
        inputCommand.selectTiles(sellTiles);
        inputCommand.displayText(Ogre::ColourValue::White, "Remove tiles");
        return;
    }

    inputCommand.unselectAllTiles();

    ClientNotification *clientNotification = new ClientNotification(
        ClientNotificationType::editorAskDestroyRoomTiles);
    uint32_t nbTiles = sellTiles.size();
    clientNotification->mPacket << nbTiles;
    for(Tile* tile : sellTiles)
        gameMap->tileToPacket(clientNotification->mPacket, tile);

    ODClient::getSingleton().queueClientNotification(clientNotification);
}

void RoomManager::sellRoomTilesEditor(GameMap* gameMap, ODPacket& packet)
{
    uint32_t nbTiles;
    OD_ASSERT_TRUE(packet >> nbTiles);
    std::set<Room*> rooms;
    std::vector<Tile*> tiles;
    while(nbTiles > 0)
    {
        --nbTiles;
        Tile* tile = gameMap->tileFromPacket(packet);
        if(tile == nullptr)
        {
            OD_ASSERT_TRUE_MSG(false, "tile=" + Tile::displayAsString(tile));
            continue;
        }
        Room* room = tile->getCoveringRoom();
        if(room == nullptr)
            continue;

        if(!room->removeCoveredTile(tile))
        {
            OD_ASSERT_TRUE_MSG(false, "room=" + room->getName() + ", tile=" + Tile::displayAsString(tile));
            continue;
        }

        tiles.push_back(tile);
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

ClientNotification* RoomManager::createRoomClientNotification(RoomType type)
{
    ClientNotification *clientNotification = new ClientNotification(ClientNotificationType::askBuildRoom);
    clientNotification->mPacket << type;
    return clientNotification;
}

ClientNotification* RoomManager::createRoomClientNotificationEditor(RoomType type)
{
    ClientNotification *clientNotification = new ClientNotification(ClientNotificationType::editorAskBuildRoom);
    clientNotification->mPacket << type;
    return clientNotification;
}
