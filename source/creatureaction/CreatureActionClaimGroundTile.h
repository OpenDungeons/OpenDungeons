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

#ifndef CREATUREACTIONCLAIMGROUNDTILE_H
#define CREATUREACTIONCLAIMGROUNDTILE_H

#include "creatureaction/CreatureAction.h"

class Tile;

class CreatureActionClaimGroundTile : public CreatureAction
{
public:
    CreatureActionClaimGroundTile(Creature& creature, Tile& tileClaim);
    virtual ~CreatureActionClaimGroundTile();

    CreatureActionType getType() const override
    { return CreatureActionType::claimGroundTile; }

    std::function<bool()> action() override;

    static bool handleCreatureActionClaimGroundTile(Creature& creature, Tile& tileClaim);

private:
    Tile& mTileClaim;
};

#endif // CREATUREACTIONCLAIMGROUNDTILE_H
