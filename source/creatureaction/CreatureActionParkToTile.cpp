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

#include "creatureaction/CreatureActionParkToTile.h"

#include "entities/Creature.h"

#include <functional>


CreatureActionParkToTile::CreatureActionParkToTile(Creature& creature) :
    CreatureAction(creature)
{
    mCreature.parkingBit = true;
    mCreature.parkedBit = false;
}

CreatureActionParkToTile::~CreatureActionParkToTile()
{
    mCreature.parkingBit = false;
    mCreature.parkedBit = true;
}

std::function<bool()> CreatureActionParkToTile::action()
{
    return std::bind(&CreatureActionParkToTile::handleParkToTile,
        std::ref(mCreature));
}

bool CreatureActionParkToTile::handleParkToTile(Creature& creature)
{
    if (creature.isMoving() && !creature.parkedBit)
        return false;

    creature.popAction();
    return true;
}
