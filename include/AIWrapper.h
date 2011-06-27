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

class Player;
class Seat;
class GameMap;

class AIWrapper
{

public:
    AIWrapper();
    virtual ~AIWrapper();
    int getGoldInTreasury();
    //Do we need more than a true/false here?
    boolean buildRoom(Room::RoomType newRoomType, int x1, int y1, int x2, int y2);
    boolean dropCreature(int x, int y, int index);
    const std::vector<const Creature*> getCreaturesInHand();
    
    //Should remove these when we have the needed functions
    const GameMap& getGameMap() {return *gameMap;}
    const Player& getPlayer() {return *player;}
    
private:
    AIWrapper(const AIWrapper& other);
    virtual AIWrapper& operator=(const AIWrapper& other);
    //virtual bool operator==(const AIWrapper& other) const;
    Player* player;
    Seat* seat;
    GameMap* gameMap;
};

#endif // AIWRAPPER_H
