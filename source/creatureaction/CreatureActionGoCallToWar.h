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

#ifndef CREATUREACTIONGOCALLTOWAR_H
#define CREATUREACTIONGOCALLTOWAR_H

#include "creatureaction/CreatureAction.h"
#include "entities/CreatureMoodValues.h"

class CreatureActionGoCallToWar : public CreatureAction
{
public:
    CreatureActionGoCallToWar(Creature& creature) :
        CreatureAction(creature)
    {}

    virtual ~CreatureActionGoCallToWar()
    {}

    CreatureActionType getType() const override
    { return CreatureActionType::goCallToWar; }

    std::function<bool()> action() override;

    uint32_t updateMoodModifier() const override
    { return CreatureMoodValues::GoToCallToWar; }

    static bool handleWalkToTile(Creature& creature);
};

#endif // CREATUREACTIONGOCALLTOWAR_H
