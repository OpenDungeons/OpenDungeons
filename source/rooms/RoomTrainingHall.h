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

#ifndef ROOMTRAININGHALL_H
#define ROOMTRAININGHALL_H

#include "rooms/Room.h"
#include "rooms/RoomType.h"

class Creature;

class RoomTrainingHall: public Room
{
public:
    RoomTrainingHall(GameMap* gameMap);

    ~RoomTrainingHall()
    {}

    RoomType getType() const override
    { return mRoomType; }

    void doUpkeep() override;
    bool hasOpenCreatureSpot(Creature* c) override;
    bool addCreatureUsingRoom(Creature* c) override;
    void removeCreatureUsingRoom(Creature* c) override;
    void absorbRoom(Room *r) override;
    bool useRoom(Creature& creature, bool forced) override;

    static const RoomType mRoomType;

protected:
    RenderedMovableEntity* notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile) override;
    void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile) override;
private:
    static const Ogre::Real OFFSET_CREATURE;
    static const Ogre::Real OFFSET_DUMMY;
    int32_t nbTurnsNoChangeDummies;
    void refreshCreaturesDummies();
    std::vector<Tile*> mUnusedDummies;
    std::map<Creature*,Tile*> mCreaturesDummies;
};

#endif // ROOMTRAININGHALL_H
