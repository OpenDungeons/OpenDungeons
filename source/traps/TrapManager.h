/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

//! \brief Factory class to register a new trap
class TrapFactory
{
public:
    virtual ~TrapFactory()
    {}

    virtual TrapType getTrapType() const = 0;
    virtual const std::string& getName() const = 0;
    virtual const std::string& getNameReadable() const = 0;
    virtual int getCostPerTile() const = 0;
    virtual const std::string& getMeshName() const = 0;

    virtual void checkBuildTrap(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const = 0;
    virtual bool buildTrap(GameMap* gameMap, Player* player, ODPacket& packet) const = 0;
    virtual void checkBuildTrapEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const = 0;
    virtual bool buildTrapEditor(GameMap* gameMap, ODPacket& packet) const = 0;
    virtual Trap* getTrapFromStream(GameMap* gameMap, std::istream& is) const = 0;
    virtual bool buildTrapOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles) const = 0;

    std::string formatBuildTrap(TrapType type, uint32_t price) const;

    //! \brief Computes the trap cost by checking the buildable tiles according to the given inputManager
    //! and updates the inputCommand with (price/buildable tiles)
    //! Note that traps that use checkBuildTrapDefault should also use buildTrapDefault and vice-versa
    //! to make sure everything works if the data sent/received are changed
    void checkBuildTrapDefault(GameMap* gameMap, TrapType type, const InputManager& inputManager, InputCommand& inputCommand) const;
    bool getTrapTilesDefault(std::vector<Tile*>& tiles, GameMap* gameMap, Player* player, ODPacket& packet) const;
    bool buildTrapDefault(GameMap* gameMap, Trap* trap, Seat* seat, const std::vector<Tile*>& tiles) const;
    void checkBuildTrapDefaultEditor(GameMap* gameMap, TrapType type, const InputManager& inputManager, InputCommand& inputCommand) const;
    bool buildTrapDefaultEditor(GameMap* gameMap, Trap* trap, ODPacket& packet) const;
};

class TrapManager
{
friend class TrapRegister;

public:
    static Trap* load(GameMap* gameMap, std::istream& is);
    //! \brief Handles the Trap deletion
    static void dispose(const Trap* trap);
    static void write(const Trap& trap, std::ostream& os);

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
    static bool buildTrapOnTiles(GameMap* gameMap, TrapType type, Player* player, const std::vector<Tile*>& tiles);


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

    static int costPerTile(TrapType type);

    static const std::string& getMeshFromTrapType(TrapType trapType);

    /*! \brief Creates a ClientNotification to ask for creating a trap. It fills the packet with the needed data
     * for the TrapManager to retrieve the spell (mainly the TrapType) so that the traps only have to handle their
     * specific data.
     */
    static ClientNotification* createTrapClientNotification(TrapType type);
    static ClientNotification* createTrapClientNotificationEditor(TrapType type);

    static int32_t getNeededWorkshopPointsPerTrap(TrapType trapType);

private:
    static void registerFactory(const TrapFactory* factory);
    static void unregisterFactory(const TrapFactory* factory);
};

class TrapRegister
{
public:
    TrapRegister(const TrapFactory* factoryToRegister) :
        mTrapFactory(factoryToRegister)
    {
        TrapManager::registerFactory(mTrapFactory);
    }
    ~TrapRegister()
    {
        TrapManager::unregisterFactory(mTrapFactory);
        delete mTrapFactory;
    }

    inline const TrapFactory* getTrapFactory() const
    { return mTrapFactory; }

private:
    const TrapFactory* mTrapFactory;
};

#endif // TRAPMANAGER_H
