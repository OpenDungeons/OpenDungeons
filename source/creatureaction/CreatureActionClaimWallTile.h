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

#ifndef CREATUREACTIONCLAIMWALLTILE_H
#define CREATUREACTIONCLAIMWALLTILE_H

#include "creatureaction/CreatureAction.h"

class Tile;

class CreatureActionClaimWallTile : public CreatureAction
{
public:
    CreatureActionClaimWallTile(Creature& creature, Tile& tileClaim);
    virtual ~CreatureActionClaimWallTile();

    CreatureActionType getType() const override
    { return CreatureActionType::claimWallTile; }

    std::function<bool()> action() override;

    static bool handleClaimWallTile(Creature& creature, Tile& tileClaim);

private:
    Tile& mTileClaim;
};

#endif // CREATUREACTIONCLAIMWALLTILE_H
