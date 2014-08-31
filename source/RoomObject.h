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

#ifndef ROOMOBJECT_H
#define ROOMOBJECT_H

#include "MovableGameEntity.h"
#include "ODPacket.h"

#include <string>
#include <istream>
#include <ostream>

class Room;
class GameMap;

class RoomObject: public MovableGameEntity
{
public:
    RoomObject(GameMap* gameMap, Room* nParentRoom, const std::string& nMeshName);
    RoomObject(GameMap* gameMap, Room* nParentRoom);

    static const std::string ROOMOBJECT_PREFIX;

    virtual std::string getOgreNamePrefix() { return "RoomObject_"; }

    Room* getParentRoom();

    //TODO: implment these in a good way
    bool doUpkeep()
    { return true; }

    void recieveExp(double experience)
    {}

    void takeDamage(double damage, Tile* tileTakingDamage)
    {}

    double getDefense() const
    { return 0.0; }

    double getHP(Tile *tile)
    { return 0; }

    std::vector<Tile*> getCoveredTiles()
    { return std::vector<Tile*>(); }

    static const char* getFormat();
    friend ODPacket& operator<<(ODPacket& os, RoomObject* o);
    friend ODPacket& operator>>(ODPacket& is, RoomObject* o);

    Ogre::Real mX;
    Ogre::Real mY;
    Ogre::Real mRotationAngle;

protected:
    virtual void createMeshLocal();
    virtual void destroyMeshLocal();
    virtual void deleteYourselfLocal();

private:
    Room* mParentRoom;
};

#endif // ROOMOBJECT_H
