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

#include "traps/TrapManager.h"

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
#include "traps/Trap.h"
#include "traps/TrapType.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

const std::string EMPTY_STRING;

namespace
{
    std::vector<TrapFunctions>& getTrapFunctions()
    {
        static std::vector<TrapFunctions> trapList(static_cast<uint32_t>(TrapType::nbTraps));
        return trapList;
    }
}

void TrapFunctions::checkBuildTrapFunc(GameMap* gameMap, TrapType type, const InputManager& inputManager, InputCommand& inputCommand) const
{
    if(mCheckBuildTrapFunc == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "null mCheckBuildTrap function Trap=" + Helper::toString(static_cast<uint32_t>(type)));
        return;
    }

    mCheckBuildTrapFunc(gameMap, inputManager, inputCommand);
}

bool TrapFunctions::buildTrapFunc(GameMap* gameMap, TrapType type, Player* player, ODPacket& packet) const
{
    if(mBuildTrapFunc == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "null mBuildTrapFunc function Trap=" + Helper::toString(static_cast<uint32_t>(type)));
        return false;
    }

    return mBuildTrapFunc(gameMap, player, packet);
}

void TrapFunctions::checkBuildTrapEditorFunc(GameMap* gameMap, TrapType type, const InputManager& inputManager, InputCommand& inputCommand) const
{
    if(mCheckBuildTrapEditorFunc == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "null mCheckBuildTrapEditorFund function Trap=" + Helper::toString(static_cast<uint32_t>(type)));
        return;
    }

    mCheckBuildTrapEditorFunc(gameMap, inputManager, inputCommand);
}

bool TrapFunctions::buildTrapEditorFunc(GameMap* gameMap, TrapType type, ODPacket& packet) const
{
    if(mBuildTrapEditorFunc == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "null mBuildTrapEditorFunc function Trap=" + Helper::toString(static_cast<uint32_t>(type)));
        return false;
    }

    return mBuildTrapEditorFunc(gameMap, packet);
}

Trap* TrapFunctions::getTrapFromStreamFunc(GameMap* gameMap, TrapType type, std::istream& is) const
{
    if(mGetTrapFromStreamFunc == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "null mGetTrapFromStreamFunc function Trap=" + Helper::toString(static_cast<uint32_t>(type)));
        return nullptr;
    }

    return mGetTrapFromStreamFunc(gameMap, is);
}

void TrapManager::checkBuildTrap(GameMap* gameMap, TrapType type, const InputManager& inputManager, InputCommand& inputCommand)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getTrapFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return;
    }

    TrapFunctions& trapFuncs = getTrapFunctions()[index];
    trapFuncs.checkBuildTrapFunc(gameMap, type, inputManager, inputCommand);
}

bool TrapManager::buildTrap(GameMap* gameMap, TrapType type, Player* player, ODPacket& packet)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getTrapFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return false;
    }

    TrapFunctions& trapFuncs = getTrapFunctions()[index];
    return trapFuncs.buildTrapFunc(gameMap, type, player, packet);
}

void TrapManager::checkBuildTrapEditor(GameMap* gameMap, TrapType type, const InputManager& inputManager, InputCommand& inputCommand)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getTrapFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return;
    }

    TrapFunctions& trapFuncs = getTrapFunctions()[index];
    trapFuncs.checkBuildTrapEditorFunc(gameMap, type, inputManager, inputCommand);
}

bool TrapManager::buildTrapEditor(GameMap* gameMap, TrapType type, ODPacket& packet)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getTrapFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return false;
    }

    TrapFunctions& trapFuncs = getTrapFunctions()[index];
    return trapFuncs.buildTrapEditorFunc(gameMap, type, packet);
}

Trap* TrapManager::getTrapFromStream(GameMap* gameMap, std::istream& is)
{
    TrapType type;
    OD_ASSERT_TRUE(is >> type);
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getTrapFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return nullptr;
    }

    TrapFunctions& trapFuncs = getTrapFunctions()[index];
    return trapFuncs.getTrapFromStreamFunc(gameMap, type, is);
}

const std::string& TrapManager::getTrapNameFromTrapType(TrapType type)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getTrapFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return EMPTY_STRING;
    }
    TrapFunctions& trapFuncs = getTrapFunctions()[index];
    return trapFuncs.mName;
}

TrapType TrapManager::getTrapTypeFromTrapName(const std::string& name)
{
    uint32_t nbTraps = static_cast<uint32_t>(TrapType::nbTraps);
    for(uint32_t i = 0; i < nbTraps; ++i)
    {
        TrapFunctions& trapFuncs = getTrapFunctions()[i];
        if(name.compare(trapFuncs.mName) == 0)
            return static_cast<TrapType>(i);
    }

    OD_ASSERT_TRUE_MSG(false, "Cannot find Trap name=" + name);
    return TrapType::nullTrapType;
}

void TrapManager::registerTrap(TrapType type, const std::string& name,
    TrapFunctions::CheckBuildTrapFunc checkBuildTrapFunc,
    TrapFunctions::BuildTrapFunc buildTrapFunc,
    TrapFunctions::CheckBuildTrapFunc checkBuildTrapEditorFunc,
    TrapFunctions::BuildTrapEditorFunc buildTrapEditorFunc,
    TrapFunctions::GetTrapFromStreamFunc getTrapFromStreamFunc)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getTrapFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return;
    }

    TrapFunctions& trapFuncs = getTrapFunctions()[index];
    trapFuncs.mName = name;
    trapFuncs.mCheckBuildTrapFunc = checkBuildTrapFunc;
    trapFuncs.mBuildTrapFunc = buildTrapFunc;
    trapFuncs.mCheckBuildTrapEditorFunc = checkBuildTrapEditorFunc;
    trapFuncs.mBuildTrapEditorFunc = buildTrapEditorFunc;
    trapFuncs.mGetTrapFromStreamFunc = getTrapFromStreamFunc;
}

void TrapManager::checkSellTrapTiles(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    Player* player = gameMap->getLocalPlayer();
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        // We do not differentiate between Trap and trap (because there is no way to know on client side).
        // Note that price = 0 doesn't mean that the building is not a Trap
        Tile* tile = gameMap->getTile(inputManager.mXPos, inputManager.mYPos);
        if((tile == nullptr) || (!tile->getIsBuilding()) || (tile->getSeat() != player->getSeat()))
        {
            inputCommand.unselectAllTiles();
            std::string txt = formatSellTrap(0);
            inputCommand.displayText(Ogre::ColourValue::White, txt);
            inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos, inputManager.mYPos);
            return;
        }

        uint32_t price = tile->getRefundPriceTrap();
        std::string txt = formatSellTrap(price);
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
        // FIXME: ATM, building is not refreshed when building a trap on client side. For this reason,
        // we accept any tile as soon as it is claimed by the correct seat
//        if(!tile->getIsBuilding())
//            continue;

        if(tile->getSeat() != player->getSeat())
            continue;

        sellTiles.push_back(tile);
        priceTotal += tile->getRefundPriceTrap();
    }

    if(inputManager.mCommandState == InputCommandState::building)
    {
        inputCommand.selectTiles(sellTiles);
        std::string txt = formatSellTrap(priceTotal);
        inputCommand.displayText(Ogre::ColourValue::White, txt);
        return;
    }

    ClientNotification *clientNotification = new ClientNotification(
        ClientNotificationType::askSellTrapTiles);
    uint32_t nbTiles = sellTiles.size();
    clientNotification->mPacket << nbTiles;
    for(Tile* tile : sellTiles)
        gameMap->tileToPacket(clientNotification->mPacket, tile);

    ODClient::getSingleton().queueClientNotification(clientNotification);
}

void TrapManager::sellTrapTiles(GameMap* gameMap, Seat* seatSell, ODPacket& packet)
{
    uint32_t nbTiles;
    OD_ASSERT_TRUE(packet >> nbTiles);
    int32_t price = 0;
    std::set<Trap*> traps;
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
        Trap* trap = tile->getCoveringTrap();
        if(trap == nullptr)
            continue;

        if(!trap->canSeatSellBuilding(seatSell))
            continue;

        if(!trap->removeCoveredTile(tile))
        {
            OD_ASSERT_TRUE_MSG(false, "trap=" + trap->getName() + ", tile=" + Tile::displayAsString(tile) + ", seatId=" + Helper::toString(seatSell->getId()));
            continue;
        }

        price += costPerTile(trap->getType()) / 2;
        tiles.push_back(tile);
        traps.insert(trap);
    }

    gameMap->addGoldToSeat(price, seatSell->getId());

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

    // We update active spots of each impacted traps
    for(Trap* trap : traps)
        trap->updateActiveSpots();
}

std::string TrapManager::formatSellTrap(int price)
{
    return "retrieve " + Helper::toString(price) + " gold";
}

void TrapManager::checkSellTrapTilesEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        // We do not differentiate between Trap and trap (because there is no way to know on client side).
        // Note that price = 0 doesn't mean that the building is not a Trap
        Tile* tile = gameMap->getTile(inputManager.mXPos, inputManager.mYPos);
        if((tile == nullptr) || (!tile->getIsBuilding()))
        {
            inputCommand.unselectAllTiles();
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
        // FIXME: ATM, building is not refreshed when building a trap on client side. For this reason,
        // we accept any tile as soon as it is claimed by the correct seat
//        if(!tile->getIsBuilding())
//            continue;

        sellTiles.push_back(tile);
    }

    if(inputManager.mCommandState == InputCommandState::building)
    {
        inputCommand.selectTiles(sellTiles);
        inputCommand.displayText(Ogre::ColourValue::White, "Remove tiles");
        return;
    }

    ClientNotification *clientNotification = new ClientNotification(
        ClientNotificationType::editorAskDestroyTrapTiles);
    uint32_t nbTiles = sellTiles.size();
    clientNotification->mPacket << nbTiles;
    for(Tile* tile : sellTiles)
        gameMap->tileToPacket(clientNotification->mPacket, tile);

    ODClient::getSingleton().queueClientNotification(clientNotification);
}

void TrapManager::sellTrapTilesEditor(GameMap* gameMap, ODPacket& packet)
{
    uint32_t nbTiles;
    OD_ASSERT_TRUE(packet >> nbTiles);
    std::set<Trap*> traps;
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
        Trap* trap = tile->getCoveringTrap();
        if(trap == nullptr)
            continue;

        if(!trap->removeCoveredTile(tile))
        {
            OD_ASSERT_TRUE_MSG(false, "trap=" + trap->getName() + ", tile=" + Tile::displayAsString(tile));
            continue;
        }

        tiles.push_back(tile);
        traps.insert(trap);
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

    // We update active spots of each impacted Traps
    for(Trap* trap : traps)
        trap->updateActiveSpots();
}

int TrapManager::costPerTile(TrapType t)
{
    switch (t)
    {
        case TrapType::nullTrapType:
            return 0;

        case TrapType::cannon:
            return ConfigManager::getSingleton().getTrapConfigInt32("CannonCostPerTile");

        case TrapType::spike:
            return ConfigManager::getSingleton().getTrapConfigInt32("SpikeCostPerTile");

        case TrapType::boulder:
            return ConfigManager::getSingleton().getTrapConfigInt32("BoulderCostPerTile");

        case TrapType::doorWooden:
            return ConfigManager::getSingleton().getTrapConfigInt32("WoodenDoorCostPerTile");

        default:
        {
            OD_ASSERT_TRUE_MSG(false, "Unknown enum for getting trap cost " + getTrapNameFromTrapType(t));
            return 0;
        }
    }
}

ClientNotification* TrapManager::createTrapClientNotification(TrapType type)
{
    ClientNotification *clientNotification = new ClientNotification(ClientNotificationType::askBuildTrap);
    clientNotification->mPacket << type;
    return clientNotification;
}

ClientNotification* TrapManager::createTrapClientNotificationEditor(TrapType type)
{
    ClientNotification *clientNotification = new ClientNotification(ClientNotificationType::editorAskBuildTrap);
    clientNotification->mPacket << type;
    return clientNotification;
}

int32_t TrapManager::getNeededWorkshopPointsPerTrap(TrapType trapType)
{
    switch(trapType)
    {
        case TrapType::nullTrapType:
            return 0;
        case TrapType::cannon:
            return ConfigManager::getSingleton().getTrapConfigInt32("CannonWorkshopPointsPerTile");
        case TrapType::spike:
            return ConfigManager::getSingleton().getTrapConfigInt32("SpikeWorkshopPointsPerTile");
        case TrapType::boulder:
            return ConfigManager::getSingleton().getTrapConfigInt32("BoulderWorkshopPointsPerTile");
        case TrapType::doorWooden:
            return ConfigManager::getSingleton().getTrapConfigInt32("WoodenDoorPointsPerTile");
        default:
            OD_ASSERT_TRUE_MSG(false, "Asked for wrong trap type=" + getTrapNameFromTrapType(trapType));
            break;
    }
    // We shouldn't go here
    return 0;
}
