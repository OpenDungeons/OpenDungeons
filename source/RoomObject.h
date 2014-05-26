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

#include <string>
#include <istream>
#include <ostream>

#include "MovableGameEntity.h"

class Room;
class GameMap;

class RoomObject: public MovableGameEntity
{
public:
    RoomObject(Room* nParentRoom, const std::string& nMeshName);

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

    std::string getOgreNamePrefix();

    static const char* getFormat();
    friend std::ostream& operator<<(std::ostream& os, RoomObject* o);
    friend std::istream& operator>>(std::istream& is, RoomObject* o);

    Ogre::Real mX;
    Ogre::Real mY;
    Ogre::Real mRotationAngle;

private:
    Room* mParentRoom;
};

#endif // ROOMOBJECT_H
