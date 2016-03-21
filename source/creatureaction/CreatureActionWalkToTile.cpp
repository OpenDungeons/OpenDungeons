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

#include "creatureaction/CreatureActionWalkToTile.h"

#include "entities/Creature.h"

#include <functional>

std::function<bool()> CreatureActionWalkToTile::action()
{
    return std::bind(&CreatureActionWalkToTile::handleWalkToTile,
        std::ref(mCreature));
}

bool CreatureActionWalkToTile::handleWalkToTile(Creature& creature)
{
    if (creature.isMoving())
        return false;

    creature.popAction();
    return true;
}
