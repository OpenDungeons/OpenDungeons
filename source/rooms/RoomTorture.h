/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#ifndef ROOMTORTURE_H
#define ROOMTORTURE_H

#include "rooms/Room.h"
#include "rooms/RoomType.h"

class Creature;
class Tile;

class RoomTortureCreatureInfo
{
public:
    RoomTortureCreatureInfo() :
        mCreature(nullptr),
        mIsReady(false),
        mState(0)
    {}

    Creature* mCreature;
    bool mIsReady;
    uint32_t mState;
};

class RoomTorture: public Room
{
public:
    RoomTorture(GameMap* gameMap);

    RoomType getType() const override
    { return mRoomType; }

    bool shouldStopUseIfHungrySleepy(Creature& creature, bool forced) override
    { return false; }
    bool shouldNotUseIfBadMood(Creature& creature, bool forced) override
    { return false; }

    void doUpkeep() override;
    bool hasOpenCreatureSpot(Creature* creature) override;
    bool addCreatureUsingRoom(Creature* creature) override;
    void removeCreatureUsingRoom(Creature* creature) override;
    void absorbRoom(Room* room) override;
    bool useRoom(Creature& creature, bool forced) override;

    bool isInContainment(Creature& creature) override;

    void creatureDropped(Creature& creature) override;

    void restoreInitialEntityState() override;

    static const RoomType mRoomType;

protected:
    BuildingObject* notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile) override;
    void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile) override;
    void exportToStream(std::ostream& os) const override;
    bool importFromStream(std::istream& is) override;

private:
    std::map<Tile*,RoomTortureCreatureInfo> mCreaturesSpots;
    std::vector<std::string> mPrisonersLoad;
};

#endif // ROOMTORTURE_H
