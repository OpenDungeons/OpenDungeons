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

#ifndef ROOM_H
#define ROOM_H

#include "entities/Building.h"

#include <string>
#include <iosfwd>

class GameMap;
class InputCommand;
class InputManager;
class ODPacket;
class Seat;
class RenderedMovableEntity;

enum class RoomType;

class Room : public Building
{
public:
    // When room types are added to this enum they also need to be added to the switch statements in Room.cpp.

    // Constructors and operators
    Room(GameMap* gameMap);
    virtual ~Room()
    {}

    virtual GameEntityType getObjectType() const
    { return GameEntityType::room; }

    virtual bool isBridge() const
    { return false; }

    virtual void addToGameMap();
    virtual void removeFromGameMap() override;

    virtual void absorbRoom(Room* r);

    static std::string getRoomStreamFormat();

    virtual RoomType getType() const = 0;

    static std::string formatBuildRoom(RoomType type, uint32_t price);

    //! \brief Computes the room cost by checking the buildable tiles according to the given inputManager
    //! and updates the inputCommand with (price/buildable tiles)
    //! Note that rooms that use checkBuildRoomDefault should also use buildRoomDefault and vice-versa
    //! to make sure everything works if the data sent/received are changed
    static void checkBuildRoomDefault(GameMap* gameMap, RoomType type, const InputManager& inputManager, InputCommand& inputCommand);
    static bool getRoomTilesDefault(std::vector<Tile*>& tiles, GameMap* gameMap, Player* player, ODPacket& packet);
    static bool buildRoomDefault(GameMap* gameMap, Room* room, Seat* seat, const std::vector<Tile*>& tiles);
    static void checkBuildRoomDefaultEditor(GameMap* gameMap, RoomType type, const InputManager& inputManager, InputCommand& inputCommand);
    static bool buildRoomDefaultEditor(GameMap* gameMap, Room* room, ODPacket& packet);

    static bool compareTile(Tile* tile1, Tile* tile2);

    //! \brief Adds a creature using the room. If the creature is allowed, true is returned
    virtual bool addCreatureUsingRoom(Creature* c);
    virtual void removeCreatureUsingRoom(Creature* c);
    virtual Creature* getCreatureUsingRoom(unsigned index);
    virtual bool hasOpenCreatureSpot(Creature* c) { return false; }

    //! \brief Updates the active spot lists.
    virtual void updateActiveSpots();

    inline unsigned int getNumActiveSpots() const
    { return mNumActiveSpots; }

    //! \brief Sets the name, seat and associates the given tiles with the room
    virtual void setupRoom(const std::string& name, Seat* seat, const std::vector<Tile*>& tiles);

    //! \brief Checks on the neighboor tiles of the room if there are other rooms of the same type/same seat.
    //! if so, it aborbs them
    void checkForRoomAbsorbtion();

    //! \brief returns true if the room can be repaired and there are destroyed tiles. false otherwise.
    bool canBeRepaired() const;

    virtual int getCostRepair();

    //! \brief Repairs the destroyed tiles of the room
    virtual void repairRoom();

    virtual void restoreInitialEntityState() override;

    static bool sortForMapSave(Room* r1, Room* r2);

protected:
    /*! \brief Exports the headers needed to recreate the Room. It allows to extend Room as much as wanted.
     * The content of the Room will be exported by exportToPacket.
     */
    virtual void exportHeadersToStream(std::ostream& os) const override;
    void exportTileDataToStream(std::ostream& os, Tile* tile, TileData* tileData) const;
    void importTileDataFromStream(std::istream& is, Tile* tile, TileData* tileData);

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

    virtual RenderedMovableEntity* notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile);
    virtual void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile);

    //! \brief This function will be called when reordering room is needed (for example if another room has been absorbed)
    static void reorderRoomTiles(std::vector<Tile*>& tiles);
private :
    void activeSpotCheckChange(ActiveSpotPlace place, const std::vector<Tile*>& originalSpotTiles,
        const std::vector<Tile*>& newSpotTiles);

};

#endif // ROOM_H
