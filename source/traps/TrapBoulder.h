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

#ifndef TRAPBOULDER_H
#define TRAPBOULDER_H

#include "traps/Trap.h"
#include "traps/TrapType.h"

class TrapBoulder : public Trap
{
public:
    static const std::string MESH_BOULDER;

    TrapBoulder(GameMap* gameMap);

    virtual const TrapType getType() const
    { return TrapType::boulder; }

    virtual bool shoot(Tile* tile);
    virtual bool isAttackable(Tile* tile, Seat* seat) const
    {
        return false;
    }

    virtual bool displayTileMesh() const override
    { return true; }

    //! \brief The boulder trap should let the ground tile visible.
    virtual bool shouldDisplayGroundTile() const
    { return true; }

    virtual TrapEntity* getTrapEntity(Tile* tile);

    static void checkBuildTrap(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);
    static bool buildTrap(GameMap* gameMap, Player* player, ODPacket& packet);
    static void checkBuildTrapEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);
    static bool buildTrapEditor(GameMap* gameMap, ODPacket& packet);
    static bool buildTrapOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles);
    static Trap* getTrapFromStream(GameMap* gameMap, std::istream& is);

    static const TrapType mTrapType;
};

#endif // TRAPBOULDER_H
