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

class GameMap;
class Player;
class Trap;
class Seat;
class Tile;

enum class TrapType;

//! Class to gather functions used for traps
class TrapFunctions
{
    friend class TrapManager;
public:
    typedef int (*GetTrapCostFunc)(std::vector<Tile*>& targets, GameMap* gameMap,
        TrapType type, int tileX1, int tileY1, int tileX2, int tileY2, Player* player);
    typedef void (*BuildTrapFunc)(GameMap*, const std::vector<Tile*>&, Seat*);
    typedef Trap* (*GetTrapFromStreamFunc)(GameMap* gameMap, std::istream& is);

    TrapFunctions() :
        mGetTrapCostFunc(nullptr),
        mBuildTrapFunc(nullptr),
        mGetTrapFromStreamFunc(nullptr)
    {}

    int getTrapCostFunc(std::vector<Tile*>& targets, GameMap* gameMap, TrapType type,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player) const;

    void buildTrapFunc(GameMap* gameMap, TrapType type, const std::vector<Tile*>& targets,
        Seat* seat) const;

    Trap* getTrapFromStreamFunc(GameMap* gameMap, TrapType type, std::istream& is) const;

private:
    std::string mName;
    GetTrapCostFunc mGetTrapCostFunc;
    BuildTrapFunc mBuildTrapFunc;
    GetTrapFromStreamFunc mGetTrapFromStreamFunc;

};

class TrapManager
{
public:
    //! Returns the Trap cost required to build the trap for the given player. targets will
    //! be filled with the suitable tiles. Note that if there are more targets than available gold,
    //! most traps will fail to build.
    //! If no target is available, this function should return the gold needed for 1 target and
    //! targets vector should be empty
    static int getTrapCost(std::vector<Tile*>& targets, GameMap* gameMap, TrapType type,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player);

    //! Builds the Trap. In most of the cases, targets should be the vector filled by getTrapCost
    static void buildTrap(GameMap* gameMap, TrapType type, const std::vector<Tile*>& targets,
        Seat* seat);

    /*! \brief Exports the headers needed to recreate the Trap. It allows to extend Traps as much as wanted.
     * The content of the Trap will be exported by exportToStream.
     */
    static Trap* getTrapFromStream(GameMap* gameMap, std::istream &is);

    static const std::string& getTrapNameFromTrapType(TrapType type);

    static TrapType getTrapTypeFromTrapName(const std::string& name);

    static int getRefundPrice(std::vector<Tile*>& tiles, GameMap* gameMap,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player);

    static void sellTrapTiles(GameMap* gameMap, const std::vector<Tile*>& tiles);

    static int costPerTile(TrapType t);

    static int32_t getNeededWorkshopPointsPerTrap(TrapType trapType);

private:
    static void registerTrap(TrapType type, const std::string& name,
        TrapFunctions::GetTrapCostFunc getTrapCostFunc,
        TrapFunctions::BuildTrapFunc buildTrapFunc,
        TrapFunctions::GetTrapFromStreamFunc getTrapFromStreamFunc);

    template <typename D>
    static int getTrapCostReg(std::vector<Tile*>& targets, GameMap* gameMap, TrapType type,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player)
    {
        return D::getTrapCost(targets, gameMap, type, tileX1, tileY1, tileX2, tileY2, player);
    }

    template <typename D>
    static void buildTrapReg(GameMap* gameMap, const std::vector<Tile*>& targets, Seat* seat)
    {
        D::buildTrap(gameMap, targets, seat);
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
    TrapManagerRegister(TrapType type, const std::string& name)
    {
        TrapManager::registerTrap(type, name, &TrapManager::getTrapCostReg<T>,
            &TrapManager::buildTrapReg<T>, &TrapManager::getTrapFromStreamReg<T>);
    }

private:
    TrapManagerRegister(const std::string& name, const TrapManagerRegister&);
};


#endif // TRAPMANAGER_H
