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

    RoomType getType() const override
    { return mRoomType; }

    void doUpkeep() override;
    bool hasOpenCreatureSpot(Creature* c) override;
    bool isRestRoom(Creature& creature) override
    { return true; }

    bool useRoom(Creature& creature, bool forced) override;
    void handleCreatureUsingAbsorbedRoom(Creature& creature) override;

    static const RoomType mRoomType;

protected:
    BuildingObject* notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile) override;
    void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile) override;
private:
    uint32_t getNbChickens();
    uint32_t mSpawnChickenCooldown;
};

#endif // ROOMHATCHERY_H
