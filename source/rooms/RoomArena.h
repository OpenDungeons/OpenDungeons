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

#ifndef ROOMARENA_H
#define ROOMARENA_H

#include "rooms/Room.h"
#include "rooms/RoomType.h"

class InputCommand;
class InputManager;

class RoomArena: public Room, public GameEntityListener
{
public:
    RoomArena(GameMap* gameMap);

    virtual RoomType getType() const override
    { return mRoomType; }

    void absorbRoom(Room *r) override;
    bool hasOpenCreatureSpot(Creature* c) override;
    bool addCreatureUsingRoom(Creature* c) override;
    void removeCreatureUsingRoom(Creature* c) override;
    void doUpkeep() override;

    bool shouldStopUseIfHungrySleepy(Creature& creature, bool forced);

    std::string getListenerName() const override;
    bool notifyDead(GameEntity* entity) override;
    bool notifyRemovedFromGameMap(GameEntity* entity) override;
    bool notifyPickedUp(GameEntity* entity) override;
    bool notifyDropped(GameEntity* entity) override;

    static const RoomType mRoomType;

protected:
    virtual BuildingObject* notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile);
    virtual void exportToStream(std::ostream& os) const override;
    virtual bool importFromStream(std::istream& is) override;

private:
    Creature* mCreatureFighting1;
    Creature* mCreatureFighting2;
};

#endif // ROOMARENA_H
