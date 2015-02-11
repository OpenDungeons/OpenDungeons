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

#include "creaturemood/CreatureMoodHunger.h"

#include "entities/Creature.h"

int32_t CreatureMoodHunger::computeMood(const Creature* creature) const
{
    int32_t hunger = static_cast<int32_t>(creature->getHunger());
    if(hunger < mStartHunger)
        return 0;

    return (hunger - mStartHunger) * mMoodModifier;
}
