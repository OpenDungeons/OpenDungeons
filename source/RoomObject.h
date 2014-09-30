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

// TODO : change name to GameObject as it is not linked to rooms anymore
class RoomObject: public MovableGameEntity
{
public:
    //! \brief Creates a room object. It's name is built from baseName and some unique id from the gamemap.
    //! We use baseName to help understand what's this object for when getting a log
    RoomObject(GameMap* gameMap, const std::string& baseName, const std::string& nMeshName);
    RoomObject(GameMap* gameMap);

    static const std::string ROOMOBJECT_PREFIX;

    virtual std::string getOgreNamePrefix() { return "RoomObject_"; }

    virtual void doUpkeep()
    {}

    void receiveExp(double experience)
    {}

    void takeDamage(GameEntity* attacker, double damage, Tile* tileTakingDamage)
    {}

    double getDefense() const
    { return 0.0; }

    double getHP(Tile *tile) const
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
};

#endif // ROOMOBJECT_H
