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

#ifndef TREASURYOBJECT_H
#define TREASURYOBJECT_H

#include "RoomObject.h"
#include "ODPacket.h"

#include <string>
#include <istream>
#include <ostream>

class Room;
class GameMap;

class TreasuryObject: public RoomObject
{
public:
    TreasuryObject(GameMap* gameMap, int goldValue);
    TreasuryObject(GameMap* gameMap);

    virtual void doUpkeep();

    virtual RoomObjectType getRoomObjectType()
    { return RoomObjectType::treasuryObject; }

    virtual bool tryPickup(Seat* seat, bool isEditorMode);
    virtual bool tryDrop(Seat* seat, Tile* tile, bool isEditorMode);
    void mergeGold(TreasuryObject* obj);
    void addGold(int goldValue);

    virtual void exportToPacket(ODPacket& packet);
    virtual void pickup();
    virtual void setPosition(const Ogre::Vector3& v);

    static const char* getFormat();
    static TreasuryObject* getTreasuryObjectFromStream(GameMap* gameMap, std::istream& is);
    static TreasuryObject* getTreasuryObjectFromPacket(GameMap* gameMap, ODPacket& packet);
    friend ODPacket& operator<<(ODPacket& os, TreasuryObject* obj);
    friend ODPacket& operator>>(ODPacket& os, TreasuryObject* obj);
    friend std::ostream& operator<<(std::ostream& os, TreasuryObject* obj);
private:
    int mGoldValue;
};

#endif // TREASURYOBJECT_H
