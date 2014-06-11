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

#include <string>
#include <deque>
#include <iostream>

class Seat;
class RoomObject;
class GameMap;

const double defaultRoomTileHP = 10.0;

class Room : public Building
{
public:
    // When room types are added to this enum they also need to be added to the switch statements in Room.cpp.
    enum RoomType
    {
        nullRoomType = 0,
        dungeonTemple,
        quarters,
        treasury,
        portal,
        forge,
        dojo,
        library
    };

    // Constructors and operators
    Room();
    virtual ~Room()
    {}

    /*! \brief Creates a type specific subclass of room (quarters, treasury, etc) and returns a pointer to it.  This function
     *  also initializes a unique default name for the room and sets up some of the room's properties.
     */
    static Room* createRoom(RoomType nType, const std::vector<Tile*> &nCoveredTiles, int nColor);

    /** \brief Builds a room for the current player.
     *  Builds a room for the current player. Checks if the player has enough gold,
     *  if not, NULL is returned.
     *  \return The room built, or NULL if the player does not have enough gold.
     */
    static Room* buildRoom(GameMap* gameMap, RoomType nType, const std::vector<Tile*> &coveredTiles,
                           Player* player, bool inEditor = false);

    /*! \brief Moves all the covered tiles from room r into this one, the rooms should be of the same subtype.
     *  After this is called the other room should likely be removed from the game map and deleted.
     *  Note that Room objects are moved to the absorbing room.
     */
    virtual void absorbRoom(Room* r);

    static std::string getFormat();
    friend std::ostream& operator<<(std::ostream& os, Room *r);
    friend std::istream& operator>>(std::istream& is, Room *r);

    static Room* createRoomFromStream(const std::string& roomName, std::istream &is, GameMap* gameMap);

    /*! \brief Creates a child RoomObject mesh using the given mesh name and placing on the target tile,
     *  if the tile is NULL the object appears in the room's center, the rotation angle is given in degrees.
     */
    RoomObject* loadRoomObject(const std::string& meshName, Tile *targetTile = NULL,
                               double rotationAngle = 0.0);
    RoomObject* loadRoomObject(const std::string& meshName, Tile *targetTile,
                               double x, double y, double rotationAngle);

    void createRoomObjectMeshes();
    void destroyRoomObjectMeshes();

    const RoomType& getType() const
    {
        return mType;
    }

    static const char* getMeshNameFromRoomType(RoomType t);
    static RoomType getRoomTypeFromMeshName(const std::string& s);

    static int costPerTile(RoomType t);

    //! \brief Carry out per turn upkeep on the room.
    //! Do any generic upkeep here (i.e. any upkeep that all room types should do).
    //! All derived classes of room should call this function first during their doUpkeep() routine.
    virtual bool doUpkeep();

    virtual void addCoveredTile(Tile* t, double nHP = defaultRoomTileHP);
    virtual void removeCoveredTile(Tile* t);
    virtual Tile* getCoveredTile(unsigned index);

    /** \brief Returns all of the tiles which are part of this room,
     *  this is to conform to the AttackableObject interface.
     */
    std::vector<Tile*> getCoveredTiles()
    {
        return mCoveredTiles;
    }

    virtual unsigned int numCoveredTiles()
    {
        return mCoveredTiles.size();
    }

    virtual void clearCoveredTiles()
    {
        mCoveredTiles.clear();
    }

    virtual bool tileIsPassable(Tile* t)
    {
        return true;
    }


    virtual void addCreatureUsingRoom(Creature* c);
    virtual void removeCreatureUsingRoom(Creature* c);
    virtual Creature* getCreatureUsingRoom(unsigned index);

    virtual unsigned int numCreaturesUsingRoom()
    {
        return mCreaturesUsingRoom.size();
    }

    /** \brief Returns how many creatures could use this room for its intended purpose:
     *  negative numbers indicate there is no limit to the number of creatures.
     */
    int numOpenCreatureSlots()
    {
        return -1;
    }

    Tile* getCentralTile();

    // Methods inherited from AttackableObject.
    double getHP(Tile *tile);

    virtual double getDefense() const
    {
        return 0.0;
    }

    void takeDamage(double damage, Tile *tileTakingDamage);

    //! \brief  Do nothing since Rooms do not have exp.
    void recieveExp(double /*experience*/)
    {}

    //! \brief Updates the active spot lists.
    void updateActiveSpots();

protected:
    std::vector<Tile*> mCoveredTiles;
    std::map<Tile*, double> mTileHP;
    std::map<Tile*, RoomObject*> mRoomObjects;
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
};

#endif // ROOM_H
