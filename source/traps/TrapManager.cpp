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
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "traps/Trap.h"
#include "traps/TrapType.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

const std::string EMPTY_STRING;

int TrapFunctions::getTrapCostFunc(std::vector<Tile*>& targets, GameMap* gameMap, TrapType type,
    int tileX1, int tileY1, int tileX2, int tileY2, Player* player) const
{
    if(mGetTrapCostFunc == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "null mGetTrapCostFunc function Trap=" + Helper::toString(static_cast<uint32_t>(type)));
        return 0;
    }

    return mGetTrapCostFunc(targets, gameMap, type, tileX1, tileY1, tileX2, tileY2, player);
}

void TrapFunctions::buildTrapFunc(GameMap* gameMap, TrapType type, const std::vector<Tile*>& targets,
    Seat* seat) const
{
    if(mBuildTrapFunc == nullptr)
    {
        OD_ASSERT_TRUE_MSG(false, "null mBuildTrapFunc function Trap=" + Helper::toString(static_cast<uint32_t>(type)));
        return;
    }

    mBuildTrapFunc(gameMap, targets, seat);
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

std::vector<TrapFunctions>& getTrapFunctions()
{
    static std::vector<TrapFunctions> trapList(static_cast<uint32_t>(TrapType::nbTraps));
    return trapList;
}

int TrapManager::getTrapCost(std::vector<Tile*>& targets, GameMap* gameMap, TrapType type,
    int tileX1, int tileY1, int tileX2, int tileY2, Player* player)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getTrapFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return 0;
    }

    TrapFunctions& trapFuncs = getTrapFunctions()[index];
    return trapFuncs.getTrapCostFunc(targets, gameMap, type, tileX1, tileY1, tileX2, tileY2, player);
}

void TrapManager::buildTrap(GameMap* gameMap, TrapType type, const std::vector<Tile*>& targets,
    Seat* seat)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= getTrapFunctions().size())
    {
        OD_ASSERT_TRUE_MSG(false, "type=" + Helper::toString(index));
        return;
    }

    TrapFunctions& trapFuncs = getTrapFunctions()[index];
    trapFuncs.buildTrapFunc(gameMap, type, targets, seat);
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
    TrapFunctions::GetTrapCostFunc getTrapCostFunc,
    TrapFunctions::BuildTrapFunc buildTrapFunc,
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
    trapFuncs.mGetTrapCostFunc = getTrapCostFunc;
    trapFuncs.mBuildTrapFunc = buildTrapFunc;
    trapFuncs.mGetTrapFromStreamFunc = getTrapFromStreamFunc;
}

int TrapManager::getRefundPrice(std::vector<Tile*>& tiles, GameMap* gameMap,
    int tileX1, int tileY1, int tileX2, int tileY2, Player* player)
{
    int price = 0;
    std::vector<Tile*> tilesRegion = gameMap->rectangularRegion(tileX1, tileY1, tileX2, tileY2);
    if(!gameMap->isServerGameMap())
    {
        // On client side, we don't fill tiles
        for(Tile* tile : tilesRegion)
            price += tile->getRefundPriceTrap();

        return price;
    }

    for(Tile* tile : tilesRegion)
    {
        Trap* trap = tile->getCoveringTrap();
        if(trap == nullptr)
            continue;

        if(!trap->canSeatSellBuilding(player->getSeat()))
            continue;

        tiles.push_back(tile);
        price += costPerTile(trap->getType()) / 2;
    }
    return price;
}

void TrapManager::sellTrapTiles(GameMap* gameMap, const std::vector<Tile*>& tiles)
{
    std::set<Trap*> traps;
    for(Tile* tile : tiles)
    {
        Trap* trap = tile->getCoveringTrap();
        if(trap == nullptr)
        {
            OD_ASSERT_TRUE_MSG(false, "tile=" + Tile::displayAsString(tile));
            continue;
        }
        OD_ASSERT_TRUE(trap->removeCoveredTile(tile));
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

    // We update active spots of each impacted traps
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

        default:
            return 0;
    }
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
        default:
            OD_ASSERT_TRUE_MSG(false, "Asked for wrong trap type=" + Ogre::StringConverter::toString(static_cast<int32_t>(trapType)));
            break;
    }
    // We shouldn't go here
    return 0;
}
