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
#include "game/Skill.h"
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

static const std::string EMPTY_STRING;


namespace
{
    static std::vector<const RoomFactory*>& getFactories()
    {
        static std::vector<const RoomFactory*> factory(static_cast<uint32_t>(RoomType::nbRooms), nullptr);
        return factory;
    }
}

std::string RoomFactory::formatBuildRoom(RoomType type, uint32_t price) const
{
    return "Build " + RoomManager::getRoomReadableName(type) + " [" + Helper::toString(price)+ " gold]";
}

void RoomFactory::checkBuildRoomDefault(GameMap* gameMap, RoomType type, const InputManager& inputManager, InputCommand& inputCommand) const
{
    Player* player = gameMap->getLocalPlayer();
    int32_t pricePerTarget = RoomManager::costPerTile(type);
    int32_t playerGold = static_cast<int32_t>(player->getSeat()->getGold());
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        if(playerGold < pricePerTarget)
        {
            std::string txt = formatBuildRoom(type, pricePerTarget);
            inputCommand.displayText(Ogre::ColourValue::Red, txt);
        }
        else
        {
            std::string txt = formatBuildRoom(type, pricePerTarget);
            inputCommand.displayText(Ogre::ColourValue::White, txt);
        }
        inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos,
            inputManager.mYPos);
        return;
    }

    std::vector<Tile*> buildableTiles = gameMap->getBuildableTilesForPlayerInArea(inputManager.mXPos,
        inputManager.mYPos, inputManager.mLStartDragX, inputManager.mLStartDragY, player);

    if(inputManager.mCommandState == InputCommandState::building)
        inputCommand.selectTiles(buildableTiles);

    if(buildableTiles.empty())
    {
        std::string txt = formatBuildRoom(type, 0);
        inputCommand.displayText(Ogre::ColourValue::White, txt);
        return;
    }

    int32_t priceTotal = static_cast<int32_t>(buildableTiles.size()) * pricePerTarget;
    if(playerGold < priceTotal)
    {
        std::string txt = formatBuildRoom(type, priceTotal);
        inputCommand.displayText(Ogre::ColourValue::Red, txt);
        return;
    }

    std::string txt = formatBuildRoom(type, priceTotal);
    inputCommand.displayText(Ogre::ColourValue::White, txt);

    if(inputManager.mCommandState != InputCommandState::validated)
        return;

    ClientNotification *clientNotification = RoomManager::createRoomClientNotification(type);
    uint32_t nbTiles = buildableTiles.size();
    clientNotification->mPacket << nbTiles;
    for(Tile* tile : buildableTiles)
        gameMap->tileToPacket(clientNotification->mPacket, tile);

    ODClient::getSingleton().queueClientNotification(clientNotification);
}

void RoomFactory::checkBuildRoomDefaultEditor(GameMap* gameMap, RoomType type, const InputManager& inputManager, InputCommand& inputCommand) const
{
    std::string txt = RoomManager::getRoomReadableName(type);
    inputCommand.displayText(Ogre::ColourValue::White, txt);
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos,
            inputManager.mYPos);
        return;
    }

    std::vector<Tile*> tiles = gameMap->rectangularRegion(inputManager.mXPos,
        inputManager.mYPos, inputManager.mLStartDragX, inputManager.mLStartDragY);

    std::vector<Tile*> buildableTiles;
    for(Tile* tile : tiles)
    {
        // We accept any tile if there is no building
        if(tile->getIsBuilding())
            continue;

        buildableTiles.push_back(tile);
    }
    if(inputManager.mCommandState == InputCommandState::building)
    {
        inputCommand.selectTiles(buildableTiles);
        return;
    }

    ClientNotification *clientNotification = RoomManager::createRoomClientNotificationEditor(type);
    uint32_t nbTiles = buildableTiles.size();
    int32_t seatId = inputManager.mSeatIdSelected;
    clientNotification->mPacket << seatId;
    clientNotification->mPacket << nbTiles;
    for(Tile* tile : buildableTiles)
        gameMap->tileToPacket(clientNotification->mPacket, tile);

    ODClient::getSingleton().queueClientNotification(clientNotification);
}

bool RoomFactory::getRoomTilesDefault(std::vector<Tile*>& tiles, GameMap* gameMap, Player* player, ODPacket& packet) const
{
    uint32_t nbTiles;
    OD_ASSERT_TRUE(packet >> nbTiles);

    while(nbTiles > 0)
    {
        --nbTiles;
        Tile* tile = gameMap->tileFromPacket(packet);
        if(tile == nullptr)
        {
            OD_LOG_ERR("unexpected null tile");
            return false;
        }

        if(!tile->isBuildableUpon(player->getSeat()))
        {
            OD_LOG_ERR("tile=" + Tile::displayAsString(tile) + ", seatId=" + Helper::toString(player->getSeat()->getId()));
            continue;
        }

        tiles.push_back(tile);
    }

    return true;
}

bool RoomFactory::buildRoomDefault(GameMap* gameMap, Room* room, Seat* seat, const std::vector<Tile*>& tiles) const
{
    if(tiles.empty())
        return false;

    room->setupRoom(gameMap->nextUniqueNameRoom(room->getType()), seat, tiles);
    room->addToGameMap();
    room->createMesh();

    if((seat->getPlayer() != nullptr) &&
       (seat->getPlayer()->getIsHuman()))
    {
        // We notify the clients with vision of the changed tiles. Note that we need
        // to calculate per seat since they could have vision on different parts of the building
        std::map<Seat*,std::vector<Tile*>> tilesPerSeat;
        const std::vector<Seat*>& seats = gameMap->getSeats();
        for(Seat* tmpSeat : seats)
        {
            if(tmpSeat->getPlayer() == nullptr)
                continue;
            if(!tmpSeat->getPlayer()->getIsHuman())
                continue;

            for(Tile* tile : tiles)
            {
                if(!tmpSeat->hasVisionOnTile(tile))
                    continue;

                tile->changeNotifiedForSeat(tmpSeat);
                tilesPerSeat[tmpSeat].push_back(tile);
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
                tile->exportToPacketForUpdate(serverNotification.mPacket, p.first);
            }
            ODServer::getSingleton().sendAsyncMsg(serverNotification);
        }
    }

    room->checkForRoomAbsorbtion();
    room->updateActiveSpots();

    return true;
}

bool RoomFactory::buildRoomDefaultEditor(GameMap* gameMap, Room* room, ODPacket& packet) const
{
    int32_t seatId;
    OD_ASSERT_TRUE(packet >> seatId);
    Seat* seatRoom = gameMap->getSeatById(seatId);
    if(seatRoom == nullptr)
    {
        OD_LOG_ERR("seatId=" + Helper::toString(seatId));
        return false;
    }

    std::vector<Tile*> tiles;
    uint32_t nbTiles;
    OD_ASSERT_TRUE(packet >> nbTiles);

    while(nbTiles > 0)
    {
        --nbTiles;
        Tile* tile = gameMap->tileFromPacket(packet);
        if(tile == nullptr)
        {
            OD_LOG_ERR("unexpected null tile room=" + room->getName());
            return false;
        }

        // If the tile is not buildable, we change it
        if(tile->getCoveringBuilding() != nullptr)
        {
            OD_LOG_ERR("tile=" + Tile::displayAsString(tile) + ", seatId=" + Helper::toString(seatId));
            continue;
        }

        tiles.push_back(tile);
        if((tile->getType() != TileType::gold) &&
        (tile->getType() != TileType::dirt))
        {
            tile->setType(TileType::dirt);
        }
        tile->setFullness(0.0);
        tile->claimTile(seatRoom);
        tile->computeTileVisual();
    }


    return buildRoomDefault(gameMap, room, seatRoom, tiles);
}

void RoomManager::registerFactory(const RoomFactory* factory)
{
    std::vector<const RoomFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(factory->getRoomType());
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return;
    }

    factories[index] = factory;
}

void RoomManager::unregisterFactory(const RoomFactory* factory)
{
    std::vector<const RoomFactory*>& factories = getFactories();
    auto it = std::find(factories.begin(), factories.end(), factory);
    if(it == factories.end())
    {
        OD_LOG_ERR("Trying to unregister unknown factory=" + factory->getName());
        return;
    }
    factories.erase(it);
}

Room* RoomManager::load(GameMap* gameMap, std::istream& is)
{
    if(!is.good())
        return nullptr;

    std::vector<const RoomFactory*>& factories = getFactories();
    std::string nextParam;
    OD_ASSERT_TRUE(is >> nextParam);
    const RoomFactory* factoryToUse = nullptr;
    for(const RoomFactory* factory : factories)
    {
        if(factory == nullptr)
            continue;

        if(factory->getName().compare(nextParam) != 0)
            continue;

        factoryToUse = factory;
        break;
    }

    if(factoryToUse == nullptr)
    {
        OD_LOG_ERR("Unknown room type=" + nextParam);
        return nullptr;
    }

    Room* room = factoryToUse->getRoomFromStream(gameMap, is);
    if(!room->importFromStream(is))
    {
        OD_LOG_ERR("Couldn't load creature room type=" + nextParam);
        delete room;
        return nullptr;
    }

    return room;
}

void RoomManager::dispose(const Room* room)
{
    delete room;
}

void RoomManager::write(const Room& room, std::ostream& os)
{
    os << room.getName();
    room.exportToStream(os);
}

void RoomManager::checkBuildRoom(GameMap* gameMap, RoomType type, const InputManager& inputManager, InputCommand& inputCommand)
{
    std::vector<const RoomFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return;
    }

    const RoomFactory& factory = *factories[index];
    factory.checkBuildRoom(gameMap, inputManager, inputCommand);
}

bool RoomManager::buildRoom(GameMap* gameMap, RoomType type, Player* player, ODPacket& packet)
{
    std::vector<const RoomFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return false;
    }

    const RoomFactory& factory = *factories[index];
    return factory.buildRoom(gameMap, player, packet);
}

void RoomManager::checkBuildRoomEditor(GameMap* gameMap, RoomType type, const InputManager& inputManager, InputCommand& inputCommand)
{
    std::vector<const RoomFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return;
    }

    const RoomFactory& factory = *factories[index];
    factory.checkBuildRoomEditor(gameMap, inputManager, inputCommand);
}

bool RoomManager::buildRoomEditor(GameMap* gameMap, RoomType type, ODPacket& packet)
{
    std::vector<const RoomFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return false;
    }

    const RoomFactory& factory = *factories[index];
    return factory.buildRoomEditor(gameMap, packet);
}

Room* RoomManager::getRoomFromStream(GameMap* gameMap, std::istream& is)
{
    RoomType type;
    if(!(is >> type))
        return nullptr;

    std::vector<const RoomFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return nullptr;
    }

    const RoomFactory& factory = *factories[index];
    return factory.getRoomFromStream(gameMap, is);
}

bool RoomManager::buildRoomOnTiles(GameMap* gameMap, RoomType type, Player* player, const std::vector<Tile*>& tiles)
{
    std::vector<const RoomFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return false;
    }

    const RoomFactory& factory = *factories[index];
    return factory.buildRoomOnTiles(gameMap, player, tiles);
}

const std::string& RoomManager::getRoomNameFromRoomType(RoomType type)
{
    std::vector<const RoomFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return EMPTY_STRING;
    }

    const RoomFactory& factory = *factories[index];
    return factory.getName();
}

const std::string& RoomManager::getRoomReadableName(RoomType type)
{
    std::vector<const RoomFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return EMPTY_STRING;
    }

    const RoomFactory& factory = *factories[index];
    return factory.getNameReadable();
}

RoomType RoomManager::getRoomTypeFromRoomName(const std::string& name)
{
    std::vector<const RoomFactory*>& factories = getFactories();
    for(const RoomFactory* factory : factories)
    {
        if(factory == nullptr)
            continue;

        if(factory->getName().compare(name) != 0)
            continue;

        return factory->getRoomType();
    }

    OD_LOG_ERR("Cannot find Room name=" + name);
    return RoomType::nullRoomType;
}

void RoomManager::checkSellRoomTiles(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    Player* player = gameMap->getLocalPlayer();
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        // We do not differentiate between room and trap (because there is no way to know on client side).
        // Note that price = 0 doesn't mean that the building is not a room
        Tile* tile = gameMap->getTile(inputManager.mXPos, inputManager.mYPos);
        if((tile == nullptr) || (!tile->getIsRoom()) || (tile->getSeat() != player->getSeat()))
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
        if(!tile->getIsRoom())
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
            OD_LOG_ERR("tile=" + Tile::displayAsString(tile));
            continue;
        }
        Room* room = tile->getCoveringRoom();
        if(room == nullptr)
            continue;

        if(!room->canSeatSellBuilding(player->getSeat()))
            continue;

        if(!room->removeCoveredTile(tile))
        {
            OD_LOG_ERR("room=" + room->getName() + ", tile=" + Tile::displayAsString(tile) + ", seatId=" + Helper::toString(player->getSeat()->getId()));
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
            tile->exportToPacketForUpdate(serverNotification.mPacket, p.first);
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
        if((tile == nullptr) || (!tile->getIsRoom()))
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
        if(!tile->getIsRoom())
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
            OD_LOG_ERR("tile=" + Tile::displayAsString(tile));
            continue;
        }
        Room* room = tile->getCoveringRoom();
        if(room == nullptr)
            continue;

        if(!room->removeCoveredTile(tile))
        {
            OD_LOG_ERR("room=" + room->getName() + ", tile=" + Tile::displayAsString(tile));
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
            tile->exportToPacketForUpdate(serverNotification.mPacket, p.first);
        }
        ODServer::getSingleton().sendAsyncMsg(serverNotification);
    }

    // We update active spots of each impacted rooms
    for(Room* room : rooms)
        room->updateActiveSpots();
}

int RoomManager::costPerTile(RoomType type)
{
    std::vector<const RoomFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return false;
    }

    const RoomFactory& factory = *factories[index];
    return factory.getCostPerTile();
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
