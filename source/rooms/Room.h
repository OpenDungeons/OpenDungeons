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

#ifndef ROOM_H
#define ROOM_H

#include "entities/Building.h"
#include "entities/Tile.h"

#include <string>
#include <deque>
#include <iostream>

class Seat;
class RenderedMovableEntity;
class GameMap;
class ODPacket;

class Room : public Building
{
public:
    // When room types are added to this enum they also need to be added to the switch statements in Room.cpp.
    enum RoomType
    {
        nullRoomType = 0,
        dungeonTemple,
        dormitory,
        treasury,
        portal,
        forge,
        trainingHall,
        library,
        hatchery
    };

    // Constructors and operators
    Room(GameMap* gameMap);
    virtual ~Room()
    {}

    virtual std::string getOgreNamePrefix() const { return "Room_"; }

    virtual void absorbRoom(Room* r);

    static std::string getFormat();

    static Room* getRoomFromStream(GameMap* gameMap, std::istream& is);
    static Room* getRoomFromPacket(GameMap* gameMap, ODPacket& is);

    /*! \brief Exports the headers needed to recreate the Room. It allows to extend Room as much as wanted.
     * The content of the Room will be exported by exportToPacket.
     */
    virtual void exportHeadersToStream(std::ostream& os);
    virtual void exportHeadersToPacket(ODPacket& os);
    //! \brief Exports the data of the RenderedMovableEntity
    virtual void exportToStream(std::ostream& os);
    virtual void importFromStream(std::istream& is);
    virtual void exportToPacket(ODPacket& os);
    virtual void importFromPacket(ODPacket& is);

    void createBuildingObjectMeshes();
    void destroyBuildingObjectMeshes();

    virtual RoomType getType() const = 0;

    static const char* getRoomNameFromRoomType(RoomType t);

    static int costPerTile(RoomType t);

    static bool compareTile(Tile* tile1, Tile* tile2);

    //! \brief Carry out per turn upkeep on the room.
    //! Do any generic upkeep here (i.e. any upkeep that all room types should do).
    //! All derived classes of room should call this function first during their doUpkeep() routine.
    virtual void doUpkeep();

    virtual void addCoveredTile(Tile* t, double nHP, bool isRoomAbsorb);
    virtual bool removeCoveredTile(Tile* t, bool isRoomAbsorb);
    virtual void addCoveredTile(Tile* t, double nHP);
    virtual bool removeCoveredTile(Tile* t);

    //! \brief Adds a creature using the room. If the creature is allowed, true is returned
    virtual bool addCreatureUsingRoom(Creature* c);
    virtual void removeCreatureUsingRoom(Creature* c);
    virtual Creature* getCreatureUsingRoom(unsigned index);
    virtual bool hasOpenCreatureSpot(Creature* c) { return false; }

    //! \brief Updates the active spot lists.
    void updateActiveSpots();

    //! \brief Sets the name, seat and associates the given tiles with the room
    void setupRoom(const std::string& name, Seat* seat, const std::vector<Tile*>& tiles);

    //! \brief Checks on the neighboor tiles of the room if there are other rooms of the same type/same seat.
    //! if so, it aborbs them
    void checkForRoomAbsorbtion();

    static bool sortForMapSave(Room* r1, Room* r2);

    friend std::istream& operator>>(std::istream& is, Room::RoomType& rt);
    friend std::ostream& operator<<(std::ostream& os, const Room::RoomType& rt);
    friend ODPacket& operator>>(ODPacket& is, Room::RoomType& rt);
    friend ODPacket& operator<<(ODPacket& os, const Room::RoomType& rt);

protected:
    enum ActiveSpotPlace
    {
        activeSpotCenter,
        activeSpotTop,
        activeSpotBottom,
        activeSpotLeft,
        activeSpotRight
    };
    std::vector<Creature*> mCreaturesUsingRoom;

    //! \brief Lists the active spots in the middle of 3x3 squares.
    std::vector<Tile*> mCentralActiveSpotTiles;
    std::vector<Tile*> mLeftWallsActiveSpotTiles;
    std::vector<Tile*> mRightWallsActiveSpotTiles;
    std::vector<Tile*> mTopWallsActiveSpotTiles;
    std::vector<Tile*> mBottomWallsActiveSpotTiles;

    //! \brief The number of active spots.
    unsigned int mNumActiveSpots;

    virtual void createMeshLocal();
    virtual void destroyMeshLocal();
    virtual RenderedMovableEntity* notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile);
    virtual void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile);
private :
    void activeSpotCheckChange(ActiveSpotPlace place, const std::vector<Tile*>& originalSpotTiles,
        const std::vector<Tile*>& newSpotTiles);

};

#endif // ROOM_H
