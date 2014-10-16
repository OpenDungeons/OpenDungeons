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

#ifndef TRAPCANNON_H
#define TRAPCANNON_H

#include "ProximityTrap.h"

class ODPacket;

class TrapCannon : public ProximityTrap
{
public:
    TrapCannon(GameMap* gameMap);

    virtual const TrapType getType() const
    { return TrapType::cannon; }

    virtual bool shoot(Tile* tile);
    virtual bool shouldDisplayMeshOnGround()
    {
        return false;
    }
    virtual RenderedMovableEntity* notifyActiveSpotCreated(Tile* tile);

    static TrapCannon* getTrapCannonFromPacket(GameMap* gameMap, ODPacket& packet);

private:
    double mCannonHeight;
};

#endif // TRAPCANNON_H

