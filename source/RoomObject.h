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

#include <string>
#include <istream>
#include <ostream>

class Room;
class GameMap;
class Seat;
class ODPacket;

// TODO : change name to RenderedMovableEntity as it is not linked to rooms anymore
class RoomObject: public MovableGameEntity
{
public:
    enum RoomObjectType
    {
        roomObject,
        treasuryObject
    };
    //! \brief Creates a room object. It's name is built from baseName and some unique id from the gamemap.
    //! We use baseName to help understand what's this object for when getting a log
    RoomObject(GameMap* gameMap, const std::string& baseName, const std::string& nMeshName, Ogre::Real rotationAngle);
    RoomObject(GameMap* gameMap);

    static const std::string ROOMOBJECT_PREFIX;
    static const std::string ROOMOBJECT_OGRE_PREFIX;

    virtual std::string getOgreNamePrefix() const { return ROOMOBJECT_OGRE_PREFIX; }

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

    Ogre::Real getRotationAngle()
    { return mRotationAngle; }

    virtual RoomObjectType getRoomObjectType()
    { return RoomObjectType::roomObject; }

    virtual bool tryPickup(Seat* seat, bool isEditorMode)
    { return false; }

    virtual bool tryDrop(Seat* seat, Tile* tile, bool isEditorMode)
    { return false; }

    virtual void pickup();
    virtual void setPosition(const Ogre::Vector3& v);

    virtual void exportToPacket(ODPacket& packet);

    static RoomObject* getRoomObjectFromLine(GameMap* gameMap, const std::string& line);
    static RoomObject* getRoomObjectFromPacket(GameMap* gameMap, ODPacket& is);
    static const char* getFormat();
    friend ODPacket& operator<<(ODPacket& os, RoomObject* o);
    friend ODPacket& operator>>(ODPacket& is, RoomObject* o);

    friend ODPacket& operator<<(ODPacket& os, const RoomObject::RoomObjectType& rot);
    friend ODPacket& operator>>(ODPacket& is, RoomObject::RoomObjectType& rot);
    friend std::ostream& operator<<(std::ostream& os, const RoomObject::RoomObjectType& rot);
    friend std::istream& operator>>(std::istream& is, RoomObject::RoomObjectType& rot);

protected:
    virtual bool getIsOnMap()
    { return mIsOnMap; }

    virtual void createMeshLocal();
    virtual void destroyMeshLocal();
    virtual void deleteYourselfLocal();
    Ogre::Real mRotationAngle;
private:
    bool mIsOnMap;
};

#endif // ROOMOBJECT_H
