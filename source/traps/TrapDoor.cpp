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

#include "traps/TrapDoor.h"

#include "entities/Creature.h"
#include "entities/DoorEntity.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/Tile.h"
#include "entities/TrapEntity.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "modes/InputCommand.h"
#include "modes/InputManager.h"
#include "network/ODClient.h"
#include "traps/TrapManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/Random.h"
#include "utils/LogManager.h"

static TrapManagerRegister<TrapDoor> reg(TrapType::doorWooden, "DoorWooden");

const std::string TrapDoor::MESH_DOOR = "WoodenDoor";
const std::string TrapDoor::ANIMATION_OPEN = "Open";
const std::string TrapDoor::ANIMATION_CLOSE = "Close";

TrapDoor::TrapDoor(GameMap* gameMap) :
    Trap(gameMap),
    mIsLocked(false)
{
    mReloadTime = 0;
    mMinDamage = 0;
    mMaxDamage = 0;
    mNbShootsBeforeDeactivation = -1;
    setMeshName("DoorWooden");
}

TrapEntity* TrapDoor::getTrapEntity(Tile* tile)
{
    Ogre::Real rotation = 90.0;
    Tile* tileW = getGameMap()->getTile(tile->getX() - 1, tile->getY());
    Tile* tileE = getGameMap()->getTile(tile->getX() + 1, tile->getY());

    if((tileW != nullptr) &&
       (tileE != nullptr) &&
       tileW->isFullTile() &&
       tileE->isFullTile())
    {
        rotation = 0.0;
    }
    return new DoorEntity(getGameMap(), true, getSeat(), getName(), MESH_DOOR, tile, rotation, false, isActivated(tile) ? 1.0f : 0.7f,
        ANIMATION_OPEN, false);
}

void TrapDoor::activate(Tile* tile)
{
    Trap::activate(tile);
    getGameMap()->doorLock(tile, getSeat(), mIsLocked);
}

void TrapDoor::doUpkeep()
{
    for(Tile* tile : mCoveredTiles)
    {
        if(!canDoorBeOnTile(getGameMap(), tile))
        {
            if(mTileData.count(tile) <= 0)
            {
                OD_LOG_ERR("trap=" + getName() + ", tile=" + Tile::displayAsString(tile));
                return;
            }

            TrapTileData* trapTileData = static_cast<TrapTileData*>(mTileData.at(tile));
            trapTileData->mHP = 0.0;
        }

        // We need to look for destroyed door before calling Trap::doUpkeep otherwise, they will be removed
        // from covered tiles
        if (mTileData[tile]->mHP <= 0.0)
        {
            getGameMap()->doorLock(tile, getSeat(), false);
        }
    }

    Trap::doUpkeep();
}

void TrapDoor::notifyDoorSlapped(DoorEntity* doorEntity, Tile* tile)
{
    mIsLocked = !mIsLocked;

    if(mIsLocked)
        doorEntity->setAnimationState(ANIMATION_CLOSE, false);
    else
        doorEntity->setAnimationState(ANIMATION_OPEN, false);

    if(!isActivated(tile))
        return;

    getGameMap()->doorLock(tile, getSeat(), mIsLocked);
}

void TrapDoor::checkBuildTrap(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    Player* player = gameMap->getLocalPlayer();
    TrapType type = TrapType::doorWooden;
    // We only allow 1 tile for door trap
    Tile* tile = gameMap->getTile(inputManager.mXPos, inputManager.mYPos);
    if(tile == nullptr)
    {
        inputCommand.unselectAllTiles();
        return;
    }

    int32_t pricePerTarget = TrapManager::costPerTile(type);
    int32_t playerGold = static_cast<int32_t>(player->getSeat()->getGold());
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        if(playerGold < pricePerTarget)
        {
            std::string txt = formatTrapPrice(type, pricePerTarget);
            inputCommand.displayText(Ogre::ColourValue::Red, txt);
        }
        else
        {
            std::string txt = formatTrapPrice(type, pricePerTarget);
            inputCommand.displayText(Ogre::ColourValue::White, txt);
        }
        inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos,
            inputManager.mYPos);
        return;
    }

    if(inputManager.mCommandState == InputCommandState::building)
    {
        std::vector<Tile*> tiles;
        tiles.push_back(tile);
        inputCommand.selectTiles(tiles);
        if(!tile->isBuildableUpon(player->getSeat()) ||
           !canDoorBeOnTile(gameMap, tile))
        {
            inputCommand.displayText(Ogre::ColourValue::Red, "Cannot place door on this tile");
        }
        else if(playerGold < pricePerTarget)
        {
            std::string txt = formatTrapPrice(type, pricePerTarget);
            inputCommand.displayText(Ogre::ColourValue::Red, txt);
        }
        else
        {
            std::string txt = formatTrapPrice(type, pricePerTarget);
            inputCommand.displayText(Ogre::ColourValue::White, txt);
        }
        return;
    }

    if(!tile->isBuildableUpon(player->getSeat()) ||
       !canDoorBeOnTile(gameMap, tile))
    {
        return;
    }

    ClientNotification *clientNotification = TrapManager::createTrapClientNotification(type);
    gameMap->tileToPacket(clientNotification->mPacket, tile);

    ODClient::getSingleton().queueClientNotification(clientNotification);
}

bool TrapDoor::buildTrap(GameMap* gameMap, Player* player, ODPacket& packet)
{
    Tile* tile = gameMap->tileFromPacket(packet);
    if(tile == nullptr)
        return false;

    if(!tile->isBuildableUpon(player->getSeat()))
        return false;

    if(!canDoorBeOnTile(gameMap, tile))
        return false;

    // The door tile is ok
    int32_t pricePerTarget = TrapManager::costPerTile(TrapType::doorWooden);
    if(!gameMap->withdrawFromTreasuries(pricePerTarget, player->getSeat()))
        return false;

    TrapDoor* trap = new TrapDoor(gameMap);
    std::vector<Tile*> tiles;
    tiles.push_back(tile);
    return buildTrapDefault(gameMap, trap, player->getSeat(), tiles);
}

void TrapDoor::checkBuildTrapEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    Seat* seat = gameMap->getSeatById(inputManager.mSeatIdSelected);
    if(seat == nullptr)
    {
        OD_LOG_ERR("seatId=" + Helper::toString(inputManager.mSeatIdSelected));
        return;
    }

    TrapType type = TrapType::doorWooden;
    // We only allow 1 tile for door trap
    Tile* tile = gameMap->getTile(inputManager.mXPos, inputManager.mYPos);
    if(tile == nullptr)
    {
        inputCommand.unselectAllTiles();
        return;
    }

    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        std::string txt = TrapManager::getTrapNameFromTrapType(type);
        inputCommand.displayText(Ogre::ColourValue::White, txt);
        inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos,
            inputManager.mYPos);
        return;
    }

    if(inputManager.mCommandState == InputCommandState::building)
    {
        std::vector<Tile*> tiles;
        tiles.push_back(tile);
        inputCommand.selectTiles(tiles);
        // We accept any tile if there is no building and there are 2 full surrounding tiles
        if(tile->getIsBuilding() ||
           !canDoorBeOnTile(gameMap, tile))
        {
            inputCommand.displayText(Ogre::ColourValue::Red, "Cannot place door on this tile");
        }
        else
        {
            std::string txt = TrapManager::getTrapNameFromTrapType(type);
            inputCommand.displayText(Ogre::ColourValue::White, txt);
        }
        return;
    }

    if(!canDoorBeOnTile(gameMap, tile))
        return;

    ClientNotification *clientNotification = TrapManager::createTrapClientNotificationEditor(type);
    int32_t seatId = inputManager.mSeatIdSelected;
    clientNotification->mPacket << seatId;
    gameMap->tileToPacket(clientNotification->mPacket, tile);

    ODClient::getSingleton().queueClientNotification(clientNotification);
}

bool TrapDoor::buildTrapEditor(GameMap* gameMap, ODPacket& packet)
{
    int32_t seatId;
    OD_ASSERT_TRUE(packet >> seatId);
    Seat* seatTrap = gameMap->getSeatById(seatId);
    if(seatTrap == nullptr)
    {
        OD_LOG_ERR("seatId=" + Helper::toString(seatId));
        return false;
    }

    Tile* tile = gameMap->tileFromPacket(packet);
    if(tile == nullptr)
        return false;

    // If the tile is not buildable, we change it
    if(tile->getCoveringBuilding() != nullptr)
    {
        OD_LOG_ERR("tile=" + Tile::displayAsString(tile) + ", seatId=" + Helper::toString(seatId));
        return false;
    }

    if(!canDoorBeOnTile(gameMap, tile))
        return false;

    if((tile->getType() != TileType::gold) &&
       (tile->getType() != TileType::dirt))
    {
        tile->setType(TileType::dirt);
    }
    tile->setFullness(0.0);
    tile->claimTile(seatTrap);
    tile->computeTileVisual();

    std::vector<Tile*> tiles;
    tiles.push_back(tile);
    TrapDoor* trap = new TrapDoor(gameMap);
    return buildTrapDefault(gameMap, trap, seatTrap, tiles);
}

bool TrapDoor::buildTrapOnTile(GameMap* gameMap, Player* player, Tile* tile)
{
    int32_t pricePerTarget = TrapManager::costPerTile(TrapType::doorWooden);
    int32_t price = pricePerTarget;
    if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
        return false;

    std::vector<Tile*> tiles;
    tiles.push_back(tile);
    TrapDoor* trap = new TrapDoor(gameMap);
    return buildTrapDefault(gameMap, trap, player->getSeat(), tiles);
}

bool TrapDoor::canDoorBeOnTile(GameMap* gameMap, Tile* tile)
{
    // We check if the tile is suitable. It can only be built on 2 full tiles
    Tile* tileW = gameMap->getTile(tile->getX() - 1, tile->getY());
    Tile* tileE = gameMap->getTile(tile->getX() + 1, tile->getY());

    if((tileW != nullptr) &&
       (tileE != nullptr) &&
       tileW->isFullTile() &&
       tileE->isFullTile())
    {
        // Ok
        return true;
    }

    Tile* tileS = gameMap->getTile(tile->getX(), tile->getY() - 1);
    Tile* tileN = gameMap->getTile(tile->getX(), tile->getY() + 1);
    if((tileS != nullptr) &&
       (tileN != nullptr) &&
       tileS->isFullTile() &&
       tileN->isFullTile())
    {
        // Ok
        return true;
    }

    return false;
}

bool TrapDoor::permitsVision(Tile* tile)
{
    TrapTileData* trapTileData = static_cast<TrapTileData*>(mTileData.at(tile));
    if (!trapTileData->isActivated())
        return true;

    return !mIsLocked;
}

bool TrapDoor::canCreatureGoThroughTile(const Creature* creature, Tile* tile) const
{
    const TrapTileData* trapTileData = static_cast<const TrapTileData*>(mTileData.at(tile));
    if (!trapTileData->isActivated())
        return true;

    if(!mIsLocked)
        return true;

    // Enemy units can go through doors. We need that otherwise, they won't be able to
    // get to the door. But in any case, if they are not fighting, we let them go. If
    // they are fighting, we don't
    if(getSeat()->isAlliedSeat(creature->getSeat()))
        return false;

    if(creature->isActionInList(CreatureActionType::fight) ||
       creature->isActionInList(CreatureActionType::flee))
    {
        return false;
    }

    return true;
}

Trap* TrapDoor::getTrapFromStream(GameMap* gameMap, std::istream& is)
{
    return new TrapDoor(gameMap);
}
