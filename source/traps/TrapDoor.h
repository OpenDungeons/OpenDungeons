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

#ifndef TRAPDOOR_H
#define TRAPDOOR_H

#include "Trap.h"
#include "traps/TrapType.h"

class DoorEntity;

class TrapDoor : public Trap
{
public:
    TrapDoor(GameMap* gameMap);

    static const std::string MESH_DOOR;

    virtual const TrapType getType() const
    { return TrapType::doorWooden; }

    // We return true to make sure every creature with vision on the door tile can see it
    virtual bool shoot(Tile* tile) override
    { return true; }

    virtual void doUpkeep() override;

    //! \brief There is no building tile for this trap
    virtual bool shouldDisplayBuildingTile() const override
    { return false; }

    //! \brief The trap object covers the whole tile under
    //! but while it built, the ground tile still must be shown.
    virtual bool shouldDisplayGroundTile() const override
    { return true; }

    virtual void activate(Tile* tile) override;

    virtual void notifyDoorSlapped(DoorEntity* doorEntity, Tile* tile);

    virtual TrapEntity* getTrapEntity(Tile* tile) override;

    virtual bool canCreatureGoThroughTile(const Creature* creature, const Tile* tile) const override
    { return !mIsLocked; }

    //! Returns true is tiles North and South (or east and west) are suitable to have a door on the
    //! given tile
    static bool canDoorBeOnTile(GameMap* gameMap, Tile* tile);

    virtual bool permitsVision(Tile* tile) override;

    static int getTrapCost(std::vector<Tile*>& tiles, GameMap* gameMap, TrapType type,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player);
    static void buildTrap(GameMap* gameMap, const std::vector<Tile*>& tiles, Seat* seat);
    static Trap* getTrapFromStream(GameMap* gameMap, std::istream& is);

private:
    bool mIsLocked;
};

#endif // TRAPDOOR_H

