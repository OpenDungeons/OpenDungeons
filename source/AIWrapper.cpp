/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  <copyright holder> <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "AIWrapper.h"
#include "Seat.h"
#include "Player.h"
#include "GameMap.h"

AIWrapper::AIWrapper(GameMap& gameMap, Player& player)
    : gameMap(gameMap), player(player), seat(*player.getSeat())
{

}

/*
AIWrapper::AIWrapper(const AIWrapper& other)
{

}*/

AIWrapper::~AIWrapper()
{

}

bool AIWrapper::buildRoom(Room::RoomType newRoomType, int x1, int y1, int x2, int y2)
{
    std::vector<Tile*> affectedTiles = gameMap.rectangularRegion(x1, y1, x2, y2);
    std::vector<Tile*>::iterator it = affectedTiles.begin();
    while(it != affectedTiles.end())
    {
        if((*it)->getColor() != seat.getColor() || !(*it)->isBuildableUpon()) {
            it = affectedTiles.erase(it);
        }
        else
        {
            ++it;
        }
    }
    Room* room = Room::buildRoom(&gameMap, newRoomType, affectedTiles, &player);
    return room != NULL;
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

int AIWrapper::getGoldInTreasury() const
{
    return seat.getGold();
}

AIWrapper& AIWrapper::operator=(const AIWrapper& other)
{
    return *this;
}
/*
bool AIWrapper::operator==(const AIWrapper& other) const
{
///TODO: return ...;
}*/

