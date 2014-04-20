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
#include "LogManager.h"

AIWrapper::AIWrapper(GameMap& gameMap, Player& player)
    : gameMap(gameMap), player(player), seat(*player.getSeat()), dungeonTemple(NULL)
{
}

AIWrapper::~AIWrapper()
{
}

bool AIWrapper::buildRoom(Room::RoomType newRoomType, int x1, int y1, int x2, int y2)
{
    Room* room = Room::buildRoom(&gameMap, newRoomType, getAffectedTiles(x1, y1, x2, y2), &player);
    return (room != NULL);
}

bool AIWrapper::buildTrap(Trap::TrapType newRoomType, int x1, int y1, int x2, int y2)
{
    Trap* trap = Trap::buildTrap(&gameMap, newRoomType, getAffectedTiles(x1, y1, x2, y2), &player);
    return (trap != NULL);
}

bool AIWrapper::dropCreature(int x, int y, int index)
{
    return player.dropCreature(gameMap.getTile(x, y), index);
}

bool AIWrapper::pickUpCreature(Creature* creature)
{
    player.pickUpCreature(creature);
    return true;
}

const std::vector< Creature* >& AIWrapper::getCreaturesInHand()
{
    return player.getCreaturesInHand();
}

std::vector< const Room* > AIWrapper::getOwnedRoomsByType(Room::RoomType type)
{
    const GameMap& gm = gameMap;
    return gm.getRoomsByTypeAndColor(type, seat.getColor());
}

const Room* AIWrapper::getDungeonTemple()
{
    if(dungeonTemple == NULL)
    {
        std::vector<Room*> dt = gameMap.getRoomsByTypeAndColor(Room::dungeonTemple, seat.getColor());
        if(!dt.empty())
        {
            dungeonTemple = dt.front();
        }
        else
        {
            LogManager::getSingleton().logMessage("Warning: AI wants dungeon temple, but it doesn't exist!");
            dungeonTemple = NULL;
        }
    }
    return dungeonTemple;
}

void AIWrapper::markTileForDigging(Tile* tile)
{
    tile->setMarkedForDigging(true, &player);
}

int AIWrapper::getGoldInTreasury() const
{
    return seat.getGold();
}

std::vector< Tile* > AIWrapper::getAffectedTiles(int x1, int y1, int x2, int y2)
{
    std::vector<Tile*> affectedTiles = gameMap.rectangularRegion(x1, y1, x2, y2);
    std::vector<Tile*>::iterator it = affectedTiles.begin();
    while(it != affectedTiles.end())
    {
        if((*it)->getColor() != seat.getColor() || !(*it)->isBuildableUpon())
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
