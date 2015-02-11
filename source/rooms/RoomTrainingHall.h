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

class Creature;

class RoomTrainingHall: public Room
{
public:
    RoomTrainingHall(GameMap* gameMap);

    ~RoomTrainingHall()
    {}

    virtual RoomType getType() const
    { return RoomType::trainingHall; }

    virtual void doUpkeep();
    virtual bool hasOpenCreatureSpot(Creature* c);
    virtual bool addCreatureUsingRoom(Creature* c);
    virtual void removeCreatureUsingRoom(Creature* c);
    virtual void absorbRoom(Room *r);

protected:
    virtual RenderedMovableEntity* notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile);
    virtual void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile);
private:
    static const Ogre::Real OFFSET_CREATURE;
    static const Ogre::Real OFFSET_DUMMY;
    int32_t nbTurnsNoChangeDummies;
    void refreshCreaturesDummies();
    std::vector<Tile*> mUnusedDummies;
    std::map<Creature*,Tile*> mCreaturesDummies;
};

#endif // ROOMTRAININGHALL_H
