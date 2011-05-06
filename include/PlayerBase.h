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


#ifndef PLAYERBASE_H
#define PLAYERBASE_H
#include <string>

#include "Room.h"

class Creature;
class Tile;
class Seat;

class PlayerBase
{

public:
    PlayerBase();

    virtual unsigned int numCreaturesInHand() {return 0;};
    virtual Creature *getCreatureInHand(int i){return NULL;};
    virtual void pickUpCreature(Creature *c){};
    virtual bool dropCreature(Tile *t){return NULL;};
    virtual void rotateCreaturesInHand(int n){};

    //virtual void buildRoom(Tile* topLeftTile, Tile* bottomRightTile, Room::RoomType roomType) = 0;

    const std::string& getNick() {return nick;}
    void setNick(const std::string& nick) {this->nick = nick;}
    Seat* getSeat() {return seat;}
    void setSeat(Seat* seat) {this->seat = seat;}

    virtual ~PlayerBase();
protected:
    void addCreatureToHand(Creature *c); // Private, for other classes use pickUpCreature() instead.
    void removeCreatureFromHand(int i); // Private, for other classes use dropCreature() instead.
    
    std::string nick;
    Seat* seat;
    std::vector<Creature*> creaturesInHand;
    bool humanPlayer;
private:
    PlayerBase(const PlayerBase& other);
    virtual PlayerBase& operator=(const PlayerBase& other);
    virtual bool operator==(const PlayerBase& other) const;
};

#endif // PLAYERBASE_H
