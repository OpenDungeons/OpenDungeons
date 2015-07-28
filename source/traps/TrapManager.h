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

#ifndef TRAPMANAGER_H
#define TRAPMANAGER_H

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
class Tile;
class Trap;

enum class TrapType;

//! Class to gather functions used for traps. Each trap should define static functions allowing to handle them:
//! - checkBuildTrap : called on client side to define the trap data in GameMode
//! - buildTrap : called on server side to create the trap in GameMode
//! - checkBuildTrapEditor : called on client side to define the trap data in EditorMode
//! - buildTrapEditor : called on server side to create the trap in EditorMode
//! - getTrapFromStream : called on server side when loading a level
class TrapFunctions
{
    friend class TrapManager;
public:
    typedef void (*CheckBuildTrapFunc)(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);
    typedef bool (*BuildTrapFunc)(GameMap* gameMap, Player* player, ODPacket& packet);
    typedef bool (*BuildTrapEditorFunc)(GameMap* gameMap, ODPacket& packet);
    typedef Trap* (*GetTrapFromStreamFunc)(GameMap* gameMap, std::istream& is);

    TrapFunctions() :
        mCheckBuildTrapFunc(nullptr),
        mBuildTrapFunc(nullptr),
        mCheckBuildTrapEditorFunc(nullptr),
        mBuildTrapEditorFunc(nullptr),
        mGetTrapFromStreamFunc(nullptr)
    {}

    void checkBuildTrapFunc(GameMap* gameMap, TrapType type, const InputManager& inputManager, InputCommand& inputCommand) const;

    bool buildTrapFunc(GameMap* gameMap, TrapType type, Player* player, ODPacket& packet) const;

    void checkBuildTrapEditorFunc(GameMap* gameMap, TrapType type, const InputManager& inputManager, InputCommand& inputCommand) const;

    bool buildTrapEditorFunc(GameMap* gameMap, TrapType type, ODPacket& packet) const;

    Trap* getTrapFromStreamFunc(GameMap* gameMap, TrapType type, std::istream& is) const;

private:
    std::string mName;
    std::string mReadableName;
    CheckBuildTrapFunc mCheckBuildTrapFunc;
    BuildTrapFunc mBuildTrapFunc;
    CheckBuildTrapFunc mCheckBuildTrapEditorFunc;
    BuildTrapEditorFunc mBuildTrapEditorFunc;
    GetTrapFromStreamFunc mGetTrapFromStreamFunc;

};

class TrapManager
{
public:
    //! \brief Called on client side. It should check if the trap can be built according to the given inputManager
    //! for the given player. It should update the InputCommand to make sure it displays the correct
    //! information (price, selection icon, ...).
    //! An ODPacket should be sent to the server if the trap is validated with relevant data. On server side, buildTrap
    //! will be called with the data from the client and it build the trap if it is validated.
    static void checkBuildTrap(GameMap* gameMap, TrapType type, const InputManager& inputManager, InputCommand& inputCommand);

    //! \brief Called on server side. Builds the trap according to the information in the packet
    //! returns true if the trap was correctly built and false otherwise
    static bool buildTrap(GameMap* gameMap, TrapType type, Player* player, ODPacket& packet);

    //! \brief Same as previous functions but for EditorMode
    static void checkBuildTrapEditor(GameMap* gameMap, TrapType type, const InputManager& inputManager, InputCommand& inputCommand);
    static bool buildTrapEditor(GameMap* gameMap, TrapType type, ODPacket& packet);

    /*! \brief Exports the headers needed to recreate the Trap. It allows to extend Traps as much as wanted.
     * The content of the Trap will be exported by exportToStream.
     */
    static Trap* getTrapFromStream(GameMap* gameMap, std::istream &is);

    //! \brief Gets the trap identification name
    static const std::string& getTrapNameFromTrapType(TrapType type);

    //! \brief Gets the trap readable name
    static const std::string& getTrapReadableName(TrapType type);

    static TrapType getTrapTypeFromTrapName(const std::string& name);

    //! \brief Called on client side. It should check if there are trap tiles to sell according
    //! to the given inputManager for the given player. It should update the InputCommand to make
    //! sure it displays the correct information (returned price, selection icon, ...).
    //! An ODPacket should be sent to the server if the action is validated with relevant data. On server side,
    //! sellTrapTiles will be called with the data from the client and it should sell the tiles if it is validated.
    static void checkSellTrapTiles(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);

    //! \brief Called on server side. Sells trap tiles according to the information in the packet
    static void sellTrapTiles(GameMap* gameMap, Seat* seatSell, ODPacket& packet);

    //! \brief Same as above functions but for Editor mode
    static void checkSellTrapTilesEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);
    static void sellTrapTilesEditor(GameMap* gameMap, ODPacket& packet);

    static std::string formatSellTrap(int price);

    static int costPerTile(TrapType t);

    /*! \brief Creates a ClientNotification to ask for creating a trap. It fills the packet with the needed data
     * for the TrapManager to retrieve the spell (mainly the TrapType) so that the traps only have to handle their
     * specific data.
     */
    static ClientNotification* createTrapClientNotification(TrapType type);
    static ClientNotification* createTrapClientNotificationEditor(TrapType type);

    static int32_t getNeededWorkshopPointsPerTrap(TrapType trapType);

private:
    static void registerTrap(TrapType type, const std::string& name, const std::string& readableName,
        TrapFunctions::CheckBuildTrapFunc checkBuildTrapFunc,
        TrapFunctions::BuildTrapFunc buildTrapFunc,
        TrapFunctions::CheckBuildTrapFunc checkBuildTrapEditorFunc,
        TrapFunctions::BuildTrapEditorFunc buildTrapEditorFunc,
        TrapFunctions::GetTrapFromStreamFunc getTrapFromStreamFunc);

    template <typename D>
    static void checkBuildTrapReg(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
    {
        D::checkBuildTrap(gameMap, inputManager, inputCommand);
    }

    template <typename D>
    static bool buildTrapReg(GameMap* gameMap, Player* player, ODPacket& packet)
    {
        return D::buildTrap(gameMap, player, packet);
    }

    template <typename D>
    static void checkBuildTrapEditorReg(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
    {
        D::checkBuildTrapEditor(gameMap, inputManager, inputCommand);
    }

    template <typename D>
    static bool buildTrapEditorReg(GameMap* gameMap, ODPacket& packet)
    {
        return D::buildTrapEditor(gameMap, packet);
    }

    template <typename D>
    static Trap* getTrapFromStreamReg(GameMap* gameMap, std::istream& is)
    {
        return D::getTrapFromStream(gameMap, is);
    }

    template <typename T> friend class TrapManagerRegister;
};

template <typename T>
class TrapManagerRegister
{
public:
    TrapManagerRegister(TrapType type, const std::string& name, const std::string& readableName)
    {
        TrapManager::registerTrap(type, name, readableName, &TrapManager::checkBuildTrapReg<T>,
            &TrapManager::buildTrapReg<T>, &TrapManager::checkBuildTrapEditorReg<T>,
            &TrapManager::buildTrapEditorReg<T>, &TrapManager::getTrapFromStreamReg<T>);
    }

private:
    TrapManagerRegister(const std::string& name, const TrapManagerRegister&);
};


#endif // TRAPMANAGER_H
