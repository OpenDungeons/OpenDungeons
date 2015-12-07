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
#include "game/Skill.h"
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

static const std::string EMPTY_STRING;

namespace
{
    static std::vector<const TrapFactory*>& getFactories()
    {
        static std::vector<const TrapFactory*> factory(static_cast<uint32_t>(TrapType::nbTraps), nullptr);
        return factory;
    }
}

std::string TrapFactory::formatBuildTrap(TrapType type, uint32_t price) const
{
    return "Build " + TrapManager::getTrapReadableName(type) + " [" + Helper::toString(price)+ " gold]";
}

void TrapFactory::checkBuildTrapDefault(GameMap* gameMap, TrapType type, const InputManager& inputManager, InputCommand& inputCommand) const
{
    Player* player = gameMap->getLocalPlayer();
    int32_t pricePerTarget = TrapManager::costPerTile(type);
    int32_t playerGold = static_cast<int32_t>(player->getSeat()->getGold());
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        if(playerGold < pricePerTarget)
        {
            std::string txt = formatBuildTrap(type, pricePerTarget);
            inputCommand.displayText(Ogre::ColourValue::Red, txt);
        }
        else
        {
            std::string txt = formatBuildTrap(type, pricePerTarget);
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
        std::string txt = formatBuildTrap(type, 0);
        inputCommand.displayText(Ogre::ColourValue::White, txt);
        return;
    }

    int32_t priceTotal = static_cast<int32_t>(buildableTiles.size()) * pricePerTarget;
    if(playerGold < priceTotal)
    {
        std::string txt = formatBuildTrap(type, priceTotal);
        inputCommand.displayText(Ogre::ColourValue::Red, txt);
        return;
    }

    std::string txt = formatBuildTrap(type, priceTotal);
    inputCommand.displayText(Ogre::ColourValue::White, txt);

    if(inputManager.mCommandState != InputCommandState::validated)
        return;

    ClientNotification *clientNotification = TrapManager::createTrapClientNotification(type);
    uint32_t nbTiles = buildableTiles.size();
    clientNotification->mPacket << nbTiles;
    for(Tile* tile : buildableTiles)
        gameMap->tileToPacket(clientNotification->mPacket, tile);

    ODClient::getSingleton().queueClientNotification(clientNotification);
}

bool TrapFactory::getTrapTilesDefault(std::vector<Tile*>& tiles, GameMap* gameMap, Player* player, ODPacket& packet) const
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

bool TrapFactory::buildTrapDefault(GameMap* gameMap, Trap* trap, Seat* seat, const std::vector<Tile*>& tiles) const
{
    if(tiles.empty())
        return false;

    trap->setupTrap(gameMap->nextUniqueNameTrap(trap->getType()), seat, tiles);
    trap->addToGameMap();
    trap->createMesh();

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

    trap->updateActiveSpots();

    return true;
}

void TrapFactory::checkBuildTrapDefaultEditor(GameMap* gameMap, TrapType type, const InputManager& inputManager, InputCommand& inputCommand) const
{
    std::string txt = TrapManager::getTrapReadableName(type);
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

    ClientNotification *clientNotification = TrapManager::createTrapClientNotificationEditor(type);
    uint32_t nbTiles = buildableTiles.size();
    int32_t seatId = inputManager.mSeatIdSelected;
    clientNotification->mPacket << seatId;
    clientNotification->mPacket << nbTiles;
    for(Tile* tile : buildableTiles)
        gameMap->tileToPacket(clientNotification->mPacket, tile);

    ODClient::getSingleton().queueClientNotification(clientNotification);
}

bool TrapFactory::buildTrapDefaultEditor(GameMap* gameMap, Trap* trap, ODPacket& packet) const
{
    int32_t seatId;
    OD_ASSERT_TRUE(packet >> seatId);
    Seat* seatTrap = gameMap->getSeatById(seatId);
    if(seatTrap == nullptr)
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
            OD_LOG_ERR("unexpected null tile");
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
        tile->claimTile(seatTrap);
        tile->computeTileVisual();
    }


    return buildTrapDefault(gameMap, trap, seatTrap, tiles);
}

void TrapManager::registerFactory(const TrapFactory* factory)
{
    std::vector<const TrapFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(factory->getTrapType());
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return;
    }

    factories[index] = factory;
}

void TrapManager::unregisterFactory(const TrapFactory* factory)
{
    std::vector<const TrapFactory*>& factories = getFactories();
    auto it = std::find(factories.begin(), factories.end(), factory);
    if(it == factories.end())
    {
        OD_LOG_ERR("Trying to unregister unknown factory=" + factory->getName());
        return;
    }
    factories.erase(it);
}

Trap* TrapManager::load(GameMap* gameMap, std::istream& is)
{
    if(!is.good())
        return nullptr;

    std::vector<const TrapFactory*>& factories = getFactories();
    std::string nextParam;
    OD_ASSERT_TRUE(is >> nextParam);
    const TrapFactory* factoryToUse = nullptr;
    for(const TrapFactory* factory : factories)
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
        OD_LOG_ERR("Unknown Trap type=" + nextParam);
        return nullptr;
    }

    Trap* trap = factoryToUse->getTrapFromStream(gameMap, is);
    if(!trap->importFromStream(is))
    {
        OD_LOG_ERR("Couldn't load creature Trap type=" + nextParam);
        delete trap;
        return nullptr;
    }

    return trap;
}

void TrapManager::dispose(const Trap* trap)
{
    delete trap;
}

void TrapManager::write(const Trap& trap, std::ostream& os)
{
    os << trap.getName();
    trap.exportToStream(os);
}

void TrapManager::checkBuildTrap(GameMap* gameMap, TrapType type, const InputManager& inputManager, InputCommand& inputCommand)
{
    std::vector<const TrapFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return;
    }

    const TrapFactory& factory = *factories[index];
    factory.checkBuildTrap(gameMap, inputManager, inputCommand);
}

bool TrapManager::buildTrap(GameMap* gameMap, TrapType type, Player* player, ODPacket& packet)
{
    std::vector<const TrapFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return false;
    }

    const TrapFactory& factory = *factories[index];
    return factory.buildTrap(gameMap, player, packet);
}

void TrapManager::checkBuildTrapEditor(GameMap* gameMap, TrapType type, const InputManager& inputManager, InputCommand& inputCommand)
{
    std::vector<const TrapFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return;
    }

    const TrapFactory& factory = *factories[index];
    factory.checkBuildTrapEditor(gameMap, inputManager, inputCommand);
}

bool TrapManager::buildTrapEditor(GameMap* gameMap, TrapType type, ODPacket& packet)
{
    std::vector<const TrapFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return false;
    }

    const TrapFactory& factory = *factories[index];
    return factory.buildTrapEditor(gameMap, packet);
}

bool TrapManager::buildTrapOnTiles(GameMap* gameMap, TrapType type, Player* player, const std::vector<Tile*>& tiles)
{
    std::vector<const TrapFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return false;
    }

    const TrapFactory& factory = *factories[index];
    return factory.buildTrapOnTiles(gameMap, player, tiles);
}

Trap* TrapManager::getTrapFromStream(GameMap* gameMap, std::istream& is)
{
    TrapType type;
    if(!(is >> type))
        return nullptr;

    std::vector<const TrapFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return nullptr;
    }

    const TrapFactory& factory = *factories[index];
    return factory.getTrapFromStream(gameMap, is);
}

const std::string& TrapManager::getTrapNameFromTrapType(TrapType type)
{
    std::vector<const TrapFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return EMPTY_STRING;
    }

    const TrapFactory& factory = *factories[index];
    return factory.getName();
}

const std::string& TrapManager::getTrapReadableName(TrapType type)
{
    std::vector<const TrapFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return EMPTY_STRING;
    }

    const TrapFactory& factory = *factories[index];
    return factory.getNameReadable();
}

TrapType TrapManager::getTrapTypeFromTrapName(const std::string& name)
{
    std::vector<const TrapFactory*>& factories = getFactories();
    for(const TrapFactory* factory : factories)
    {
        if(factory == nullptr)
            continue;
        if(factory->getName().compare(name) != 0)
            continue;

        return factory->getTrapType();
    }

    OD_LOG_ERR("Cannot find Trap name=" + name);
    return TrapType::nullTrapType;
}

void TrapManager::checkSellTrapTiles(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    Player* player = gameMap->getLocalPlayer();
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        // We do not differentiate between Trap and trap (because there is no way to know on client side).
        // Note that price = 0 doesn't mean that the building is not a Trap
        Tile* tile = gameMap->getTile(inputManager.mXPos, inputManager.mYPos);
        if((tile == nullptr) || (!tile->getIsTrap()) || (tile->getSeat() != player->getSeat()))
        {
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
        if(!tile->getIsTrap())
            continue;

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

    inputCommand.unselectAllTiles();

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
            OD_LOG_ERR("tile=" + Tile::displayAsString(tile));
            continue;
        }
        Trap* trap = tile->getCoveringTrap();
        if(trap == nullptr)
            continue;

        if(!trap->canSeatSellBuilding(seatSell))
            continue;

        if(!trap->removeCoveredTile(tile))
        {
            OD_LOG_ERR("trap=" + trap->getName() + ", tile=" + Tile::displayAsString(tile) + ", seatId=" + Helper::toString(seatSell->getId()));
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
            tile->exportToPacketForUpdate(serverNotification.mPacket, p.first);
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
        inputCommand.displayText(Ogre::ColourValue::White, "Remove tiles");
        inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos, inputManager.mYPos);
        return;
    }

    std::vector<Tile*> sellTiles;
    std::vector<Tile*> tiles = gameMap->rectangularRegion(inputManager.mXPos,
        inputManager.mYPos, inputManager.mLStartDragX, inputManager.mLStartDragY);
    for(Tile* tile : tiles)
    {
        if(!tile->getIsTrap())
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
            OD_LOG_ERR("tile=" + Tile::displayAsString(tile));
            continue;
        }
        Trap* trap = tile->getCoveringTrap();
        if(trap == nullptr)
            continue;

        if(!trap->removeCoveredTile(tile))
        {
            OD_LOG_ERR("trap=" + trap->getName() + ", tile=" + Tile::displayAsString(tile));
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
            tile->exportToPacketForUpdate(serverNotification.mPacket, p.first);
        }
        ODServer::getSingleton().sendAsyncMsg(serverNotification);
    }

    // We update active spots of each impacted Traps
    for(Trap* trap : traps)
        trap->updateActiveSpots();
}

int TrapManager::costPerTile(TrapType type)
{
    std::vector<const TrapFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return 0;
    }

    const TrapFactory& factory = *factories[index];
    return factory.getCostPerTile();
}

const std::string& TrapManager::getMeshFromTrapType(TrapType type)
{
    std::vector<const TrapFactory*>& factories = getFactories();
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= factories.size())
    {
        OD_LOG_ERR("type=" + Helper::toString(index) + ", factories.size=" + Helper::toString(factories.size()));
        return EMPTY_STRING;
    }

    const TrapFactory& factory = *factories[index];
    return factory.getMeshName();
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
            OD_LOG_ERR("Asked for wrong trap type=" + getTrapNameFromTrapType(trapType));
            break;
    }
    // We shouldn't go here
    return 0;
}
