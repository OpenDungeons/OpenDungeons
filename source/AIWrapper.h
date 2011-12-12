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


#ifndef AIWRAPPER_H
#define AIWRAPPER_H

#include <vector>

#include "Room.h"
#include "Trap.h"

class Player;
class Seat;
class GameMap;
class Creature;

class AIWrapper
{

public:
    AIWrapper(GameMap& gameMap, Player& player);
    virtual ~AIWrapper();
    int getGoldInTreasury() const;
    //Do we need more than a true/false here?
    bool buildRoom(Room::RoomType newRoomType, int x1, int y1, int x2, int y2);
    bool buildTrap(Trap::TrapType newRoomType, int x1, int y1, int x2, int y2);
    bool dropCreature(int x, int y, int index);
    bool pickUpCreature(Creature* creature);
    const std::vector<Creature*>& getCreaturesInHand();
    std::vector<const Room*> getOwnedRoomsByType(Room::RoomType type);
    const Room* getDungeonTemple();
    void markTileForDigging(Tile* tile);
    std::vector<Tile*> rectangularRegion(int x1, int y1, int x2, int y2);
    std::vector<Tile*> circularRegion(int x, int y, double radius);
    
    //Should remove these when we have the needed functions
    const GameMap& getGameMap() const {return gameMap;}
    const Player& getPlayer() const {return player;}
    
private:
    AIWrapper(const AIWrapper& other);
    virtual AIWrapper& operator=(const AIWrapper& other);
    std::vector<Tile*> getAffectedTiles(int x1, int y1, int x2, int y2);
    //virtual bool operator==(const AIWrapper& other) const;
    GameMap& gameMap;
    Player& player;
    Seat& seat;

    Room* dungeonTemple;
};

#endif // AIWRAPPER_H
