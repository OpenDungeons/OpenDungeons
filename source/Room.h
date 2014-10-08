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

#include "Building.h"
#include "Tile.h"
#include "ODPacket.h"

#include <string>
#include <deque>
#include <iostream>

class Seat;
class RoomObject;
class GameMap;

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

    /*! \brief Creates a type specific subclass of room (dormitory, treasury, etc) and returns a pointer to it.
     * This function sets up some of the room's properties. If nameToUse is empty, a new unique name
     * will be generated. If not, the given one will be used
     */
    static Room* createRoom(GameMap* gameMap, RoomType nType, const std::vector<Tile*> &nCoveredTiles, Seat* seat,
        bool forceName = false, const std::string& name = "");

    /** \brief Adds the room newRoom to the game map. If the border tiles
     * contains another room of same type, it will absorb them
     */
    static void setupRoom(GameMap* gameMap, Room* newRoom);

    /*! \brief Moves all the covered tiles from room r into this one, the rooms should be of the same subtype.
     *  After this is called the other room should likely be removed from the game map and deleted.
     *  Note that Room objects are moved to the absorbing room.
     */
    virtual void absorbRoom(Room* r);

    static std::string getFormat();
    friend std::ostream& operator<<(std::ostream& os, Room *r);
    friend std::istream& operator>>(std::istream& is, Room *r);
    friend ODPacket& operator<<(ODPacket& os, Room *r);
    friend ODPacket& operator>>(ODPacket& is, Room *r);

    static Room* createRoomFromStream(GameMap* gameMap, const std::string& roomMeshName, std::istream& is,
        const std::string& roomName);
    static Room* createRoomFromPacket(GameMap* gameMap, const std::string& roomMeshName, ODPacket& is,
        const std::string& roomName);

    void createRoomObjectMeshes();
    void destroyRoomObjectMeshes();

    const RoomType& getType() const
    {
        return mType;
    }

    static const char* getMeshNameFromRoomType(RoomType t);
    static const char* getRoomNameFromRoomType(RoomType t);
    static RoomType getRoomTypeFromMeshName(const std::string& s);

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

    static bool sortForMapSave(Room* r1, Room* r2);

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
    RoomType mType;

    //! \brief Lists the active spots in the middle of 3x3 squares.
    std::vector<Tile*> mCentralActiveSpotTiles;
    std::vector<Tile*> mLeftWallsActiveSpotTiles;
    std::vector<Tile*> mRightWallsActiveSpotTiles;
    std::vector<Tile*> mTopWallsActiveSpotTiles;
    std::vector<Tile*> mBottomWallsActiveSpotTiles;

    //! \brief The number of active spots.
    unsigned int mNumActiveSpots;

protected:
    virtual void createMeshLocal();
    virtual void destroyMeshLocal();
    virtual void deleteYourselfLocal();
    virtual RoomObject* notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile);
    virtual void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile);
private :
    void activeSpotCheckChange(ActiveSpotPlace place, const std::vector<Tile*>& originalSpotTiles,
        const std::vector<Tile*>& newSpotTiles);

};

#endif // ROOM_H
