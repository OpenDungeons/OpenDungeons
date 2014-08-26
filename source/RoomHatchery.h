/*
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

#ifndef ROOMHATCHERY_H
#define ROOMHATCHERY_H

#include "Room.h"

class Creature;

class RoomHatchery: public Room
{
public:
    RoomHatchery(GameMap* gameMap);

    ~RoomHatchery()
    {}

    virtual bool doUpkeep();
    virtual bool hasOpenCreatureSpot(Creature* c);
    virtual bool addCreatureUsingRoom(Creature* c);
    virtual void removeCreatureUsingRoom(Creature* c);
    virtual void absorbRoom(Room *r);
    virtual void addCoveredTile(Tile* t, double nHP);
    virtual void removeCoveredTile(Tile* t);

protected:
    virtual RoomObject* notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile);
    virtual void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile);
private:
    void moveChickens();
    std::vector<Tile*> mUnusedTiles;
    std::vector<Tile*> mChickensFree;
    int mNbChickensEaten;
    std::map<Creature*,Tile*> mCreaturesChickens;
    int32_t mSpawnChickenCooldown;
};

#endif // ROOMHATCHERY_H
