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

#include "creaturemood/CreatureMoodHpLoss.h"

#include "entities/Creature.h"

#include "utils/Helper.h"

int32_t CreatureMoodHpLoss::computeMood(const Creature* creature) const
{
    int32_t hpLost = creature->getMaxHp() - creature->getHP();
    if(hpLost <= 0)
        return 0;

    hpLost = Helper::round(static_cast<float>(hpLost));
    return hpLost * mMoodModifier;
}
