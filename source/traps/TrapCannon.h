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

#ifndef TRAPCANNON_H
#define TRAPCANNON_H

#include "Trap.h"
#include "traps/TrapType.h"

class ODPacket;

class TrapCannon : public Trap
{
public:
    TrapCannon(GameMap* gameMap);

    virtual const TrapType getType() const override
    { return TrapType::cannon; }

    virtual bool shoot(Tile* tile) override;

    virtual bool displayTileMesh() const override
    { return true; }

    //! \brief The cannon should show the ground tile under.
    virtual bool shouldDisplayGroundTile() const override
    { return true; }

    virtual double getPhysicalDefense() const override;
    virtual double getMagicalDefense() const override;
    virtual double getElementDefense() const override;

    virtual TrapEntity* getTrapEntity(Tile* tile) override;

    static const TrapType mTrapType;

private:
    uint32_t mRange;
};

#endif // TRAPCANNON_H

