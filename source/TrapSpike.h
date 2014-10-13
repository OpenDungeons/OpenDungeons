/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#include "ProximityTrap.h"

class TrapSpike : public ProximityTrap
{
public:
    TrapSpike(GameMap* gameMap);

    virtual const TrapType getType() const
    { return TrapType::spike; }

    virtual bool shoot(Tile* tile);
    virtual bool isAttackable() const
    {
        return false;
    }

    virtual bool shouldDisplayMeshOnGround()
    {
        return false;
    }

    virtual RoomObject* notifyActiveSpotCreated(Tile* tile);
};

#endif // TRAPSPIKE_H

