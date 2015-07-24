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
class Room;
class Seat;
class Tile;

enum class RoomType;

//! Class to gather functions used for rooms. Each room should define static functions allowing to handle them:
//! - checkBuildRoom : called on client side to define the room data in GameMode
//! - buildRoom : called on server side to create the room in GameMode
//! - checkBuildRoomEditor : called on client side to define the room data in EditorMode
//! - buildRoomEditor : called on server side to create the room in EditorMode
//! - getRoomFromStream : called on server side when loading a level
class RoomFunctions
{
    friend class RoomManager;
public:
    typedef void (*CheckBuildRoomFunc)(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);
    typedef bool (*BuildRoomFunc)(GameMap* gameMap, Player* player, ODPacket& packet);
    typedef bool (*BuildRoomEditorFunc)(GameMap* gameMap, ODPacket& packet);
    typedef Room* (*GetRoomFromStreamFunc)(GameMap* gameMap, std::istream& is);

    RoomFunctions() :
        mCheckBuildRoomFunc(nullptr),
        mBuildRoomFunc(nullptr),
        mCheckBuildRoomEditorFunc(nullptr),
        mBuildRoomEditorFunc(nullptr),
        mGetRoomFromStreamFunc(nullptr)
    {}

    void checkBuildRoomFunc(GameMap* gameMap, RoomType type, const InputManager& inputManager, InputCommand& inputCommand) const;

    bool buildRoomFunc(GameMap* gameMap, RoomType type, Player* player, ODPacket& packet) const;

    void checkBuildRoomEditorFunc(GameMap* gameMap, RoomType type, const InputManager& inputManager, InputCommand& inputCommand) const;

    bool buildRoomEditorFunc(GameMap* gameMap, RoomType type, ODPacket& packet) const;

    Room* getRoomFromStreamFunc(GameMap* gameMap, RoomType type, std::istream& is) const;

private:
    std::string mName;
    CheckBuildRoomFunc mCheckBuildRoomFunc;
    BuildRoomFunc mBuildRoomFunc;
    CheckBuildRoomFunc mCheckBuildRoomEditorFunc;
    BuildRoomEditorFunc mBuildRoomEditorFunc;
    GetRoomFromStreamFunc mGetRoomFromStreamFunc;
};

class RoomManager
{
public:
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

    static const std::string& getRoomNameFromRoomType(RoomType type);

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

    static int costPerTile(RoomType t);

    /*! \brief Creates a ClientNotification to ask for creating a room. It fills the packet with the needed data
     * for the RoomManager to retrieve the spell (mainly the RoomType) so that the rooms only have to handle their
     * specific data.
     */
    static ClientNotification* createRoomClientNotification(RoomType type);
    static ClientNotification* createRoomClientNotificationEditor(RoomType type);

private:
    static void registerRoom(RoomType type, const std::string& name,
        RoomFunctions::CheckBuildRoomFunc checkBuildRoomFunc,
        RoomFunctions::BuildRoomFunc buildRoomFunc,
        RoomFunctions::CheckBuildRoomFunc checkBuildRoomEditorFunc,
        RoomFunctions::BuildRoomEditorFunc buildRoomEditorFunc,
        RoomFunctions::GetRoomFromStreamFunc getRoomFromStreamFunc);

    template <typename D>
    static void checkBuildRoomReg(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
    {
        D::checkBuildRoom(gameMap, inputManager, inputCommand);
    }

    template <typename D>
    static bool buildRoomReg(GameMap* gameMap, Player* player, ODPacket& packet)
    {
        return D::buildRoom(gameMap, player, packet);
    }

    template <typename D>
    static void checkBuildRoomEditorReg(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
    {
        D::checkBuildRoomEditor(gameMap, inputManager, inputCommand);
    }

    template <typename D>
    static bool buildRoomEditorReg(GameMap* gameMap, ODPacket& packet)
    {
        return D::buildRoomEditor(gameMap, packet);
    }

    template <typename D>
    static Room* getRoomFromStreamReg(GameMap* gameMap, std::istream& is)
    {
        return D::getRoomFromStream(gameMap, is);
    }

    template <typename T> friend class RoomManagerRegister;
};

template <typename T>
class RoomManagerRegister
{
public:
    RoomManagerRegister(RoomType type, const std::string& name)
    {
        RoomManager::registerRoom(type, name, &RoomManager::checkBuildRoomReg<T>,
            &RoomManager::buildRoomReg<T>, &RoomManager::checkBuildRoomEditorReg<T>,
            &RoomManager::buildRoomEditorReg<T>, &RoomManager::getRoomFromStreamReg<T>);
    }

private:
    RoomManagerRegister(const std::string& name, const RoomManagerRegister&);
};


#endif // ROOMMANAGER_H
