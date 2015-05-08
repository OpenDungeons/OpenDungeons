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

#ifndef ROOMHATCHERY_H
#define ROOMHATCHERY_H

#include "rooms/Room.h"
#include "rooms/RoomType.h"

class Creature;

class RoomHatchery: public Room
{
public:
    RoomHatchery(GameMap* gameMap);

    ~RoomHatchery()
    {}

    virtual RoomType getType() const
    { return RoomType::hatchery; }

    virtual void doUpkeep();
    virtual bool hasOpenCreatureSpot(Creature* c);

    static int getRoomCost(std::vector<Tile*>& tiles, GameMap* gameMap, RoomType type,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player);
    static void buildRoom(GameMap* gameMap, const std::vector<Tile*>& tiles, Seat* seat);
    static Room* getRoomFromStream(GameMap* gameMap, std::istream& is);

protected:
    virtual RenderedMovableEntity* notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile);
    virtual void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile);
private:
    uint32_t getNbChickens();
    uint32_t mSpawnChickenCooldown;
};

#endif // ROOMHATCHERY_H
