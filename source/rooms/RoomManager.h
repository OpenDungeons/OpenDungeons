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

#ifndef ROOMMANAGER_H
#define ROOMMANAGER_H

#include <vector>
#include <istream>
#include <cstdint>

class ClientNotification;
class GameMap;
class InputCommand;
class InputManager;
class ODPacket;
class Player;
class Seat;
class Room;
class Tile;

enum class RoomType;

//! \brief Factory class to register a new room
class RoomFactory
{
public:
    virtual ~RoomFactory()
    {}

    virtual RoomType getRoomType() const = 0;
    virtual const std::string& getName() const = 0;
    virtual const std::string& getNameReadable() const = 0;
    virtual int getCostPerTile() const = 0;

    virtual void checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const = 0;
    virtual bool buildRoom(GameMap* gameMap, Player* player, ODPacket& packet) const = 0;
    virtual void checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const = 0;
    virtual bool buildRoomEditor(GameMap* gameMap, ODPacket& packet) const = 0;
    virtual Room* getRoomFromStream(GameMap* gameMap, std::istream& is) const = 0;
    virtual bool buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles) const = 0;

    std::string formatBuildRoom(RoomType type, uint32_t price) const;
    //! \brief Computes the room cost by checking the buildable tiles according to the given inputManager
    //! and updates the inputCommand with (price/buildable tiles)
    //! Note that rooms that use checkBuildRoomDefault should also use buildRoomDefault and vice-versa
    //! to make sure everything works if the data sent/received are changed
    void checkBuildRoomDefault(GameMap* gameMap, RoomType type, const InputManager& inputManager, InputCommand& inputCommand) const;
    bool getRoomTilesDefault(std::vector<Tile*>& tiles, GameMap* gameMap, Player* player, ODPacket& packet) const;
    bool buildRoomDefault(GameMap* gameMap, Room* room, Seat* seat, const std::vector<Tile*>& tiles) const;
    void checkBuildRoomDefaultEditor(GameMap* gameMap, RoomType type, const InputManager& inputManager, InputCommand& inputCommand) const;
    bool buildRoomDefaultEditor(GameMap* gameMap, Room* room, ODPacket& packet) const;

};

class RoomManager
{
friend class RoomRegister;

public:
    static Room* load(GameMap* gameMap, std::istream& is);
    //! \brief Handles the Room deletion
    static void dispose(const Room* room);
    static void write(const Room& room, std::ostream& os);

    //! \brief Called on client side. It should check if the room can be built according to the given inputManager
    //! for the given player. It should update the InputCommand to make sure it displays the correct
    //! information (price, selection icon, ...).
    //! An ODPacket should be sent to the server if the room is validated with relevant data. On server side, buildRoom
    //! will be called with the data from the client and it build the room if it is validated.
    static void checkBuildRoom(GameMap* gameMap, RoomType type, const InputManager& inputManager, InputCommand& inputCommand);

    //! \brief Called on server side. Builds the room according to the information in the packet
    //! returns true if the room was correctly built and false otherwise
    static bool buildRoom(GameMap* gameMap, RoomType type, Player* player, ODPacket& packet);

    //! \brief Same as previous functions but for EditorMode
    static void checkBuildRoomEditor(GameMap* gameMap, RoomType type, const InputManager& inputManager, InputCommand& inputCommand);
    static bool buildRoomEditor(GameMap* gameMap, RoomType type, ODPacket& packet);

    //! \brief Constructs a room according to the data in the stream
    static Room* getRoomFromStream(GameMap* gameMap, std::istream &is);

    //! \brief Used by AI for building rooms
    static bool buildRoomOnTiles(GameMap* gameMap, RoomType type, Player* player, const std::vector<Tile*>& tiles);

    //! \brief Gets the room identification name
    static const std::string& getRoomNameFromRoomType(RoomType type);

    //! \brief Gets the room readable name
    static const std::string& getRoomReadableName(RoomType type);

    static RoomType getRoomTypeFromRoomName(const std::string& name);

    //! \brief Called on client side. It should check if there are room tiles to sell according
    //! to the given inputManager for the given player. It should update the InputCommand to make
    //! sure it displays the correct information (returned price, selection icon, ...).
    //! An ODPacket should be sent to the server if the action is validated with relevant data. On server side,
    //! sellRoomTiles will be called with the data from the client and it should sell the tiles if it is validated.
    static void checkSellRoomTiles(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);

    //! \brief Called on server side. Sells room tiles according to the information in the packet
    static void sellRoomTiles(GameMap* gameMap, Player* player, ODPacket& packet);

    //! \brief Same as above functions but for Editor mode
    static void checkSellRoomTilesEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);
    static void sellRoomTilesEditor(GameMap* gameMap, ODPacket& packet);

    static std::string formatSellRoom(int price);

    static int costPerTile(RoomType type);

    /*! \brief Creates a ClientNotification to ask for creating a room. It fills the packet with the needed data
     * for the RoomManager to retrieve the spell (mainly the RoomType) so that the rooms only have to handle their
     * specific data.
     */
    static ClientNotification* createRoomClientNotification(RoomType type);
    static ClientNotification* createRoomClientNotificationEditor(RoomType type);

private:
    static void registerFactory(const RoomFactory* factory);
    static void unregisterFactory(const RoomFactory* factory);
};

class RoomRegister
{
public:
    RoomRegister(const RoomFactory* factoryToRegister) :
        mRoomFactory(factoryToRegister)
    {
        RoomManager::registerFactory(mRoomFactory);
    }
    ~RoomRegister()
    {
        RoomManager::unregisterFactory(mRoomFactory);
        delete mRoomFactory;
    }

private:
    const RoomFactory* mRoomFactory;
};

#endif // ROOMMANAGER_H
