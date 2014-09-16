/*!
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#include "AIWrapper.h"

#include "Seat.h"
#include "Player.h"
#include "GameMap.h"
#include "ODServer.h"
#include "ServerNotification.h"
#include "LogManager.h"

AIWrapper::AIWrapper(GameMap& gameMap, Player& player)
    : gameMap(gameMap), player(player), seat(*player.getSeat())
{
}

AIWrapper::~AIWrapper()
{
}

bool AIWrapper::buildRoom(Room::RoomType newRoomType, int x1, int y1, int x2, int y2)
{
    std::vector<Tile*> coveredTiles = getAffectedTiles(x1, y1, x2, y2);
    int nbTiles = coveredTiles.size();
    int goldRequired = nbTiles * Room::costPerTile(newRoomType);
    if(gameMap.withdrawFromTreasuries(goldRequired, player.getSeat()))
    {
        Seat* seat = player.getSeat();
        Room* newRoom = Room::createRoom(&gameMap, newRoomType, coveredTiles, seat);
        try
        {
            int seatId = seat->getId();
            const std::string& name = newRoom->getName();
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotification::buildRoom, &player);
            int intType = static_cast<int32_t>(newRoom->getType());
            serverNotification->mPacket << name << intType << seatId << nbTiles;
            for(std::vector<Tile*>::iterator it = coveredTiles.begin(); it != coveredTiles.end(); ++it)
            {
                Tile* tile = *it;
                serverNotification->mPacket << tile;
            }
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
        catch (std::bad_alloc&)
        {
            OD_ASSERT_TRUE(false);
            exit(1);
        }
        Room::setupRoom(&gameMap, newRoom);
        return true;
    }

    return false;
}

bool AIWrapper::buildTrap(Trap::TrapType newTrapType, int x1, int y1, int x2, int y2)
{
    std::vector<Tile*> coveredTiles = getAffectedTiles(x1, y1, x2, y2);
    int goldRequired = coveredTiles.size() * Trap::costPerTile(newTrapType);
    if(gameMap.withdrawFromTreasuries(goldRequired, player.getSeat()))
    {
        Trap* newTrap = Trap::createTrap(&gameMap, newTrapType, coveredTiles, player.getSeat());
        Trap::setupTrap(&gameMap, newTrap);
        return true;
    }
    return false;
}

bool AIWrapper::dropCreature(int x, int y, int index)
{
    if(!player.isDropCreaturePossible(gameMap.getTile(x, y), index))
        return false;

    player.dropCreature(gameMap.getTile(x, y), index);
    return true;
}

bool AIWrapper::pickUpCreature(Creature* creature)
{
    player.pickUpCreature(creature);
    return true;
}

const std::vector<Creature*>& AIWrapper::getCreaturesInHand()
{
    return player.getCreaturesInHand();
}

std::vector<const Room*> AIWrapper::getOwnedRoomsByType(Room::RoomType type)
{
    const GameMap& gm = gameMap;
    return gm.getRoomsByTypeAndSeat(type, &seat);
}

Room* AIWrapper::getDungeonTemple()
{
    std::vector<Room*> dt = gameMap.getRoomsByTypeAndSeat(Room::dungeonTemple, &seat);
    if(!dt.empty())
        return dt.front();
    else
        return NULL;
}

void AIWrapper::markTileForDigging(Tile* tile)
{
    tile->setMarkedForDigging(true, &player);
}

int AIWrapper::getGoldInTreasury() const
{
    return seat.getGold();
}

std::vector<Tile*> AIWrapper::getAffectedTiles(int x1, int y1, int x2, int y2)
{
    std::vector<Tile*> affectedTiles = gameMap.rectangularRegion(x1, y1, x2, y2);
    std::vector<Tile*>::iterator it = affectedTiles.begin();
    while(it != affectedTiles.end())
    {
        if((*it)->getSeat() != &seat || !(*it)->isBuildableUpon())
        {
            it = affectedTiles.erase(it);
        }
        else
        {
            ++it;
        }
    }
    return affectedTiles;
}
