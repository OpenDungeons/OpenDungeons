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

#include "rooms/RoomBridgeWooden.h"

#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "modes/InputManager.h"
#include "network/ODPacket.h"
#include "rooms/RoomManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

const std::string RoomBridgeWoodenName = "WoodenBridge";
const std::string RoomBridgeWoodenNameDisplay = "Wooden Bridge room";
const RoomType RoomBridgeWooden::mRoomType = RoomType::bridgeWooden;
static const std::vector<TileVisual> allowedTilesVisual = {TileVisual::waterGround};

namespace
{
class RoomBridgeWoodenFactory : public BridgeRoomFactory
{
    RoomType getRoomType() const override
    { return RoomBridgeWooden::mRoomType; }

    const std::string& getName() const override
    { return RoomBridgeWoodenName; }

    const std::string& getNameReadable() const override
    { return RoomBridgeWoodenNameDisplay; }

    int getCostPerTile() const override
    { return ConfigManager::getSingleton().getRoomConfigInt32("WoodenBridgeCostPerTile"); }

    void checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        Player* player = gameMap->getLocalPlayer();
        checkBuildBridge(RoomBridgeWooden::mRoomType, gameMap, player->getSeat(), inputManager, inputCommand, allowedTilesVisual, false);
    }

    bool buildRoom(GameMap* gameMap, Player* player, ODPacket& packet) const
    {
        std::vector<Tile*> tiles;
        if(!readBridgeFromPacket(tiles, gameMap, player->getSeat(), allowedTilesVisual, packet, false))
            return false;

        return buildRoomOnTiles(gameMap, player, tiles);
    }

    void checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        Seat* seatRoom = gameMap->getSeatById(inputManager.mSeatIdSelected);
        if(seatRoom == nullptr)
        {
            OD_LOG_ERR("seatId=" + Helper::toString(inputManager.mSeatIdSelected));
            return;
        }

        checkBuildBridge(RoomBridgeWooden::mRoomType, gameMap, seatRoom, inputManager, inputCommand, allowedTilesVisual, true);
    }

    bool buildRoomEditor(GameMap* gameMap, ODPacket& packet) const override
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
        if(!readBridgeFromPacket(tiles, gameMap, seatRoom, allowedTilesVisual, packet, true))
            return false;

        RoomBridgeWooden* room = new RoomBridgeWooden(gameMap);
        return buildRoomDefault(gameMap, room, seatRoom, tiles);
    }

    Room* getRoomFromStream(GameMap* gameMap, std::istream& is) const override
    {
        RoomBridgeWooden* room = new RoomBridgeWooden(gameMap);
        if(!Room::importRoomFromStream(*room, is))
        {
            OD_LOG_ERR("Error while building a room from the stream");
        }
        return room;
    }

    bool buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles) const override
    {
        int32_t pricePerTarget = RoomManager::costPerTile(RoomBridgeWooden::mRoomType);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        RoomBridgeWooden* room = new RoomBridgeWooden(gameMap);
        return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
    }
};

// Register the factory
static RoomRegister reg(new RoomBridgeWoodenFactory);
}

RoomBridgeWooden::RoomBridgeWooden(GameMap* gameMap) :
    RoomBridge(gameMap)
{
    setMeshName("WoodBridge");
}

void RoomBridgeWooden::updateFloodFillTileRemoved(Seat* seat, Tile* tile)
{
    // Update floodfill to check if a path is broken. To do that, we will find all neighbors
    // to replace their values by new floodfill values and proceed from next to next
    std::vector<uint32_t> colorsToChange(static_cast<uint32_t>(FloodFillType::nbValues), Tile::NO_FLOODFILL);
    for(uint32_t i = 0; i < colorsToChange.size(); ++i)
    {
        FloodFillType type = static_cast<FloodFillType>(i);
        // For water tiles, we don't want to change water related floodfill
        if(tile->getTileVisual() == TileVisual::waterGround)
        {
            if((type == FloodFillType::groundWater) ||
               (type == FloodFillType::groundWaterLava))
            {
                continue;
            }
        }
        else if(tile->getTileVisual() == TileVisual::lavaGround)
        {
            if((type == FloodFillType::groundLava) ||
               (type == FloodFillType::groundWaterLava))
            {
                continue;
            }
        }
        colorsToChange[i] = tile->getFloodFillValue(seat, type);
    }

    switch(tile->getTileVisual())
    {
        case TileVisual::waterGround:
        {
            tile->replaceFloodFill(seat, FloodFillType::ground, Tile::NO_FLOODFILL);
            tile->replaceFloodFill(seat, FloodFillType::groundLava, Tile::NO_FLOODFILL);
            break;
        }
        case TileVisual::lavaGround:
        {
            tile->replaceFloodFill(seat, FloodFillType::ground, Tile::NO_FLOODFILL);
            tile->replaceFloodFill(seat, FloodFillType::groundWater, Tile::NO_FLOODFILL);
            break;
        }
        default:
        {
            // Unexpected tile visual since bridges are expected to be over lava or water
            OD_LOG_ERR("Unexpected tile visual " + Tile::tileVisualToString(tile->getTileVisual()));
            return;
        }
    }

    // To update the flood fill, we first want to separate neighbor tiles ground and not ground.
    // The idea is to find the first most walkable tile (ground if any or lava if bridge over water).
    // We do not change floodfill of the first most walkable tile (allowing to not change floodfill
    // at all if the bridge is already broken). If we find more than 1, we set a new floodfill value
    // to all other walkable tiles
    std::vector<Tile*> groundTiles;
    std::vector<Tile*> notGroundTiles;
    uint32_t groundFloodFillIndex = static_cast<uint32_t>(FloodFillType::ground);
    for(Tile* neigh : tile->getAllNeighbors())
    {
        // This will select ground walkable tiles (be it ground tile or tile with a bridge)
        if(neigh->getFloodFillValue(seat, FloodFillType::ground) == colorsToChange[groundFloodFillIndex])
        {
            groundTiles.push_back(neigh);
            continue;
        }

        // Same for groundWaterLava (works with both water and lava bridge)
        uint32_t groundWaterLavaFloodFillIndex = static_cast<uint32_t>(FloodFillType::groundWaterLava);
        if(neigh->getFloodFillValue(seat, FloodFillType::groundWaterLava) == colorsToChange[groundWaterLavaFloodFillIndex])
        {
            notGroundTiles.push_back(neigh);
            continue;
        }
    }

    bool isSkippedTileProcessed = false;
    for(Tile* neigh : groundTiles)
    {
        if(!isSkippedTileProcessed)
        {
            isSkippedTileProcessed = true;
            continue;
        }

        // We replace floodfill for the tile
        std::vector<uint32_t> newColors(static_cast<uint32_t>(FloodFillType::nbValues), Tile::NO_FLOODFILL);
        for(uint32_t i = 0; i < newColors.size(); ++i)
        {
            if(colorsToChange[i] == Tile::NO_FLOODFILL)
                continue;

            newColors[i] = getGameMap()->nextUniqueFloodFillValue();
        }
        getGameMap()->changeFloodFillConnectedTiles(neigh, seat, colorsToChange, newColors, nullptr);
    }

    // We have processed ground tiles. We can change not ground tiles
    colorsToChange[groundFloodFillIndex] = Tile::NO_FLOODFILL;
    for(Tile* neigh : notGroundTiles)
    {
        if(!isSkippedTileProcessed)
        {
            isSkippedTileProcessed = true;
            continue;
        }

        // We replace floodfill for the tile
        std::vector<uint32_t> newColors(static_cast<uint32_t>(FloodFillType::nbValues), Tile::NO_FLOODFILL);
        for(uint32_t i = 0; i < newColors.size(); ++i)
        {
            if(colorsToChange[i] == Tile::NO_FLOODFILL)
                continue;

            newColors[i] = getGameMap()->nextUniqueFloodFillValue();
        }
        getGameMap()->changeFloodFillConnectedTiles(neigh, seat, colorsToChange, newColors, nullptr);
    }
}
