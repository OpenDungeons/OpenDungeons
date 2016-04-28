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

#include "rooms/RoomBridge.h"

#include "entities/Creature.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "modes/InputCommand.h"
#include "modes/InputManager.h"
#include "network/ODClient.h"
#include "rooms/RoomManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

const double CLAIMED_VALUE_PER_TILE = 1.0;

void BridgeRoomFactory::checkBuildBridge(RoomType type, GameMap* gameMap, Seat* seat, const InputManager& inputManager,
    InputCommand& inputCommand, const std::vector<TileVisual>& allowedTilesVisual, bool isEditor) const
{
    if(isEditor)
    {
        std::string txt = RoomManager::getRoomReadableName(type);
        inputCommand.displayText(Ogre::ColourValue::White, txt);
    }

    int32_t pricePerTarget = RoomManager::costPerTile(type);
    int32_t playerGold = static_cast<int32_t>(seat->getGold());
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        if(!isEditor)
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
        }
        inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos,
            inputManager.mYPos);
        return;
    }

    // We only allow straight building
    // We only keep the tiles if they are next to another tile from the bridge or if
    // they are next to a claimed tile
    std::vector<Tile*> buildableTiles;
    int x;
    int xEnd;
    int y;
    int yEnd;
    int xDiff;
    int yDiff;
    if(inputManager.mXPos == inputManager.mLStartDragX)
    {
        // Vertical bridge
        x = inputManager.mXPos;
        xEnd = inputManager.mXPos;
        y = std::min(inputManager.mYPos, inputManager.mLStartDragY);
        yEnd = std::max(inputManager.mYPos, inputManager.mLStartDragY);
        xDiff = 0;
        yDiff = 1;
    }
    else
    {
        // Horizontal bridge
        x = std::min(inputManager.mXPos, inputManager.mLStartDragX);
        xEnd = std::max(inputManager.mXPos, inputManager.mLStartDragX);
        y = inputManager.mYPos;
        yEnd = inputManager.mYPos;
        xDiff = 1;
        yDiff = 0;
    }

    // This vector is used to store allowed bridge tiles if we don't start from a
    // claimed tile. If we find one, we will add the tiles in reverse order to make
    // sure the tiles are pushed in a buildable order
    std::vector<Tile*> tmpTiles;
    bool isClaimedTileNeighFound = isEditor;
    while((x <= xEnd) && (y <= yEnd))
    {
        Tile* tile = gameMap->getTile(x, y);
        // We keep allowed tiles without bridges only
        if((tile == nullptr) ||
           tile->getHasBridge() ||
           (std::find(allowedTilesVisual.begin(), allowedTilesVisual.end(), tile->getTileVisual()) == allowedTilesVisual.end()))
        {
            x += xDiff;
            y += yDiff;
            isClaimedTileNeighFound = false;

            tmpTiles.clear();

            // We only allow a continuous bridge. But it can start from dirt tiles (even if
            // only accepted tiles will be selected). But, we allow non accepted tiles only
            // if we have no accepted ones so far. Otherwise, we assume it is a wrong click
            if(buildableTiles.empty())
                continue;

            break;
        }

        if(!isClaimedTileNeighFound)
        {
            for(Tile* tileNeigh : tile->getAllNeighbors())
            {
                if(tileNeigh->isClaimedForSeat(seat) &&
                   !tileNeigh->isFullTile())
                {
                    isClaimedTileNeighFound = true;
                    break;
                }
            }
        }

        if(isClaimedTileNeighFound)
        {
            buildableTiles.push_back(tile);
            if(!tmpTiles.empty())
            {
                buildableTiles.insert(buildableTiles.end(), tmpTiles.rbegin(), tmpTiles.rend());
                tmpTiles.clear();
            }
        }
        else
            tmpTiles.push_back(tile);

        x += xDiff;
        y += yDiff;
    }

    if(isClaimedTileNeighFound &&
       !tmpTiles.empty())
    {
        buildableTiles.insert(buildableTiles.end(), tmpTiles.begin(), tmpTiles.end());
    }

    if(inputManager.mCommandState == InputCommandState::building)
        inputCommand.selectTiles(buildableTiles);

    if(buildableTiles.empty())
    {
        if(!isEditor)
        {
            std::string txt = formatBuildRoom(type, 0);
            inputCommand.displayText(Ogre::ColourValue::White, txt);
        }
        return;
    }

    if(!isEditor)
    {
        int32_t priceTotal = static_cast<int32_t>(buildableTiles.size()) * pricePerTarget;
        if(playerGold < priceTotal)
        {
            std::string txt = formatBuildRoom(type, priceTotal);
            inputCommand.displayText(Ogre::ColourValue::Red, txt);
            return;
        }

        std::string txt = formatBuildRoom(type, priceTotal);
        inputCommand.displayText(Ogre::ColourValue::White, txt);
    }

    if(inputManager.mCommandState != InputCommandState::validated)
        return;

    ClientNotification *clientNotification;
    if(isEditor)
    {
        clientNotification = RoomManager::createRoomClientNotificationEditor(type);
        int32_t seatId = inputManager.mSeatIdSelected;
        clientNotification->mPacket << seatId;
    }
    else
        clientNotification = RoomManager::createRoomClientNotification(type);

    uint32_t nbTiles = buildableTiles.size();
    clientNotification->mPacket << nbTiles;
    for(Tile* tile : buildableTiles)
        gameMap->tileToPacket(clientNotification->mPacket, tile);

    ODClient::getSingleton().queueClientNotification(clientNotification);
}

bool BridgeRoomFactory::readBridgeFromPacket(std::vector<Tile*>& tiles, GameMap* gameMap, Seat* seat,
    const std::vector<TileVisual>& allowedTilesVisual, ODPacket& packet, bool isEditor) const
{
    // Note that here, we expect the client to have filled requested tiles in a correct
    // order: the first tile must be next to a claimed tile, the second next to the first
    // or from a claimed tile, ...
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

        if((tile == nullptr) ||
           tile->getHasBridge() ||
           (std::find(allowedTilesVisual.begin(), allowedTilesVisual.end(), tile->getTileVisual()) == allowedTilesVisual.end()))
        {
            OD_LOG_ERR("tile=" + Tile::displayAsString(tile) + ", seatId=" + Helper::toString(seat->getId()));
            break;
        }

        const std::vector<Tile*>& neighs = tile->getAllNeighbors();
        bool isOk = isEditor;
        // We check if it is the next tile from the bridge
        if(!tiles.empty() &&
           std::find(neighs.begin(), neighs.end(), neighs.back()) != neighs.end())
        {
            isOk = true;
        }

        // If it is not part of the bridge, it has to be next to a claimed tile
        if(!isOk)
        {
            for(Tile* tileNeigh : neighs)
            {
                if(tileNeigh->isClaimedForSeat(seat))
                {
                    isOk = true;
                    break;
                }
            }
        }

        // The bridge should have been checked on client side. The tiles are expected to be sorted in a buildable order
        if(!isOk)
        {
            OD_LOG_WRN("Unexpected tile for bridge: " + Tile::displayAsString(tile));
            return false;
        }

        // If there is no claimed tile around the processed one, we check if it is the next part of a bridge
        tiles.push_back(tile);
    }

    return true;
}

RoomBridge::RoomBridge(GameMap* gameMap) :
    Room(gameMap),
    mClaimedValue(0)
{
}

void RoomBridge::setupRoom(const std::string& name, Seat* seat, const std::vector<Tile*>& tiles)
{
    Room::setupRoom(name, seat, tiles);

    mClaimedValue = static_cast<double>(tiles.size()) * CLAIMED_VALUE_PER_TILE;

    for(Seat* s : getGameMap()->getSeats())
        updateFloodFillPathCreated(s, tiles);
}

void RoomBridge::restoreInitialEntityState()
{
    Room::restoreInitialEntityState();

    for(Seat* s : getGameMap()->getSeats())
        updateFloodFillPathCreated(s, getCoveredTiles());
}

void RoomBridge::exportToStream(std::ostream& os) const
{
    Room::exportToStream(os);

    os << mClaimedValue << "\n";
}

bool RoomBridge::importFromStream(std::istream& is)
{
    if(!Room::importFromStream(is))
        return false;

    if(!(is >> mClaimedValue))
        return false;

    return true;
}

void RoomBridge::absorbRoom(Room *r)
{
    if(r->getType() != getType())
    {
        OD_LOG_ERR("Trying to merge incompatible rooms: " + getName() + ", type=" + RoomManager::getRoomNameFromRoomType(getType()) + ", with " + r->getName() + ", type=" + RoomManager::getRoomNameFromRoomType(r->getType()));
        return;
    }
    RoomBridge* oldRoom = static_cast<RoomBridge*>(r);
    mClaimedValue += oldRoom->mClaimedValue;

    Room::absorbRoom(r);
}

bool RoomBridge::removeCoveredTile(Tile* t)
{
    if(!Room::removeCoveredTile(t))
        return false;

    if(mClaimedValue > CLAIMED_VALUE_PER_TILE)
        mClaimedValue -= CLAIMED_VALUE_PER_TILE;

    for(Seat* seat : getGameMap()->getSeats())
        updateFloodFillTileRemoved(seat, t);

    return true;
}

bool RoomBridge::isClaimable(Seat* seat) const
{
    return !getSeat()->isAlliedSeat(seat);
}

void RoomBridge::claimForSeat(Seat* seat, Tile* tile, double danceRate)
{
    if(mClaimedValue > danceRate)
    {
        mClaimedValue -= danceRate;
        return;
    }

    mClaimedValue = static_cast<double>(numCoveredTiles());
    setSeat(seat);

    for(Tile* tile : mCoveredTiles)
        tile->claimTile(seat);

    // We check if by claiming this bridge, we created a bigger one (can happen
    // if a player builds a bridge next to another one's)
    checkForRoomAbsorbtion();
    updateActiveSpots();
}

double RoomBridge::getCreatureSpeed(const Creature* creature, Tile* tile) const
{
    return creature->getMoveSpeedGround();
}
