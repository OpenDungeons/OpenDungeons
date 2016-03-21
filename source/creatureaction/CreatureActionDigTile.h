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

#ifndef CREATUREACTIONDIGTILE_H
#define CREATUREACTIONDIGTILE_H

#include "creatureaction/CreatureAction.h"

class Tile;

class CreatureActionDigTile : public CreatureAction
{
public:
    CreatureActionDigTile(Creature& creature, Tile& tileDig);
    virtual ~CreatureActionDigTile();

    CreatureActionType getType() const override
    { return CreatureActionType::digTile; }

    std::function<bool()> action() override;

    static bool handleDigTile(Creature& creature, Tile& tileDig);

private:
    Tile& mTileDig;
};

#endif // CREATUREACTIONDIGTILE_H
