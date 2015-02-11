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

#include "creaturemood/CreatureMoodAwakness.h"

#include "entities/Creature.h"

int32_t CreatureMoodAwakness::computeMood(const Creature* creature) const
{
    int32_t awakness = static_cast<int32_t>(creature->getAwakeness());
    if(awakness > mStartAwakness)
        return 0;

    return (mStartAwakness - awakness) * mMoodModifier;
}
