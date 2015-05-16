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

#ifndef TRAPSPIKE_H
#define TRAPSPIKE_H

#include "Trap.h"
#include "traps/TrapType.h"

class TrapSpike : public Trap
{
public:
    static const std::string MESH_SPIKE;

    TrapSpike(GameMap* gameMap);

    virtual const TrapType getType() const
    { return TrapType::spike; }

    virtual bool shoot(Tile* tile);
    virtual bool isAttackable(Tile* tile, Seat* seat) const
    {
        return false;
    }

    //! \brief There is no building tile for this trap
    virtual bool shouldDisplayBuildingTile() const
    { return false; }

    //! \brief The trap object covers the whole tile under
    //! but while it built, the ground tile still must be shown.
    virtual bool shouldDisplayGroundTile() const
    { return true; }

    virtual TrapEntity* getTrapEntity(Tile* tile);

    static int getTrapCost(std::vector<Tile*>& tiles, GameMap* gameMap, TrapType type,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player);
    static void buildTrap(GameMap* gameMap, const std::vector<Tile*>& tiles, Seat* seat);
    static Trap* getTrapFromStream(GameMap* gameMap, std::istream& is);

};

#endif // TRAPSPIKE_H

