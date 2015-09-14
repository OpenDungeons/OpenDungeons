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

#include "rooms/RoomBridgeStone.h"

#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "modes/InputManager.h"
#include "network/ODPacket.h"
#include "rooms/RoomManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

static RoomManagerRegister<RoomBridgeStone> reg(RoomType::bridgeStone, "StoneBridge", "Stone Bridge room");

static const std::vector<TileVisual> allowedTilesVisual = {TileVisual::waterGround, TileVisual::lavaGround};

RoomBridgeStone::RoomBridgeStone(GameMap* gameMap) :
    RoomBridge(gameMap)
{
    setMeshName("StoneBridge");
}

void RoomBridgeStone::updateFloodFillPathCreated(Seat* seat, const std::vector<Tile*>& tiles)
{
    // We look for the first ground flood fill value
    std::vector<uint32_t> colors(static_cast<uint32_t>(FloodFillType::nbValues), Tile::NO_FLOODFILL);
    bool isFloodfilled = false;
    for(Tile* tile : tiles)
    {
        isFloodfilled = false;
        for(Tile* neigh : tile->getAllNeighbors())
        {
            isFloodfilled = true;
            for(uint32_t i = 0; i < colors.size(); ++i)
            {
                FloodFillType type = static_cast<FloodFillType>(i);
                if(colors[i] != Tile::NO_FLOODFILL)
                    continue;

                isFloodfilled = false;
                colors[i] = neigh->getFloodFillValue(seat, type);
            }

            if(isFloodfilled)
                break;
        }

        if(isFloodfilled)
            break;
    }

    if(!isFloodfilled)
    {
        // We couldn't find any tile with ground floodfill. That's not normal as bridges
        // are supposed to be built from a ground claimed tile
        OD_LOG_ERR("Couldn't find floodfill tile for bridge=" + getName() + ", seatId=" + Helper::toString(seat->getId()));
        return;
    }

    // Now, we update the floodfill for everytile connected to the bridge (including the
    // bridge itself)
    for(Tile* tile : tiles)
    {
        for(uint32_t i = 0; i < colors.size(); ++i)
        {
            if(colors[i] == Tile::NO_FLOODFILL)
                continue;

            FloodFillType type = static_cast<FloodFillType>(i);
            uint32_t tileColor = tile->getFloodFillValue(seat, type);
            if(tileColor != colors[i])
                tile->replaceFloodFill(seat, type, colors[i]);

            for(Tile* neigh : tile->getAllNeighbors())
            {
                uint32_t neighColor = neigh->getFloodFillValue(seat, type);
                if(neighColor == Tile::NO_FLOODFILL)
                    continue;

                if(neighColor == colors[i])
                    continue;

                getGameMap()->replaceFloodFill(seat, type, neighColor, colors[i]);
            }
        }
    }
}

void RoomBridgeStone::updateFloodFillTileRemoved(Seat* seat, Tile* tile)
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

void RoomBridgeStone::checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    Player* player = gameMap->getLocalPlayer();
    checkBuildBridge(RoomType::bridgeStone, gameMap, player->getSeat(), inputManager, inputCommand, allowedTilesVisual, false);
}

void RoomBridgeStone::checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    Seat* seatRoom = gameMap->getSeatById(inputManager.mSeatIdSelected);
    if(seatRoom == nullptr)
    {
        OD_LOG_ERR("seatId=" + Helper::toString(inputManager.mSeatIdSelected));
        return;
    }

    checkBuildBridge(RoomType::bridgeStone, gameMap, seatRoom, inputManager, inputCommand, allowedTilesVisual, true);
}

bool RoomBridgeStone::buildRoom(GameMap* gameMap, Player* player, ODPacket& packet)
{
    std::vector<Tile*> tiles;
    if(!readBridgeFromPacket(tiles, gameMap, player->getSeat(), allowedTilesVisual, packet, false))
        return false;

    return buildRoomOnTiles(gameMap, player, tiles);
}

bool RoomBridgeStone::buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles)
{
    int32_t pricePerTarget = RoomManager::costPerTile(RoomType::bridgeStone);
    int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
    if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
        return false;

    RoomBridgeStone* room = new RoomBridgeStone(gameMap);
    return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
}

bool RoomBridgeStone::buildRoomEditor(GameMap* gameMap, ODPacket& packet)
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

    RoomBridgeStone* room = new RoomBridgeStone(gameMap);
    return buildRoomDefault(gameMap, room, seatRoom, tiles);
}

Room* RoomBridgeStone::getRoomFromStream(GameMap* gameMap, std::istream& is)
{
    RoomBridgeStone* room = new RoomBridgeStone(gameMap);
    room->importFromStream(is);
    return room;
}
