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

#ifndef CREATUREACTIONSEARCHENTITYTOCARRY_H
#define CREATUREACTIONSEARCHENTITYTOCARRY_H

#include "creatureaction/CreatureAction.h"

class CreatureActionSearchEntityToCarry : public CreatureAction
{
public:
    CreatureActionSearchEntityToCarry(Creature& creature, bool forced);
    virtual ~CreatureActionSearchEntityToCarry();

    CreatureActionType getType() const override
    { return CreatureActionType::searchEntityToCarry; }

    std::function<bool()> action() override;

    static bool handleSearchEntityToCarry(Creature& creature, bool forced);

private:
    bool mForced;
};

#endif // CREATUREACTIONSEARCHENTITYTOCARRY_H
