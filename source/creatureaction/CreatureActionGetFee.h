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

#ifndef CREATUREACTIONGETFEE_H
#define CREATUREACTIONGETFEE_H

#include "creatureaction/CreatureAction.h"

class CreatureActionGetFee : public CreatureAction
{
public:
    CreatureActionGetFee(Creature& creature) :
        CreatureAction(creature)
    {}

    virtual ~CreatureActionGetFee()
    {}

    CreatureActionType getType() const override
    { return CreatureActionType::getFee; }

    uint32_t updateMoodModifier() const override
    { return CreatureMoodValues::GetFee; }

    std::function<bool()> action() override;

    static bool handleGetFee(Creature& creature);
};

#endif // CREATUREACTIONGETFEE_H
