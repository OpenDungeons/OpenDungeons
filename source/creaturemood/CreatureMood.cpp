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

#include "creaturemood/CreatureMood.h"

#include "creaturemood/CreatureMoodAwakness.h"
#include "creaturemood/CreatureMoodCreature.h"
#include "creaturemood/CreatureMoodFee.h"
#include "creaturemood/CreatureMoodHpLoss.h"
#include "creaturemood/CreatureMoodHunger.h"
#include "creaturemood/CreatureMoodTurnsWithoutFight.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <istream>

std::string CreatureMood::toString(CreatureMoodLevel moodLevel)
{
    switch(moodLevel)
    {
        case CreatureMoodLevel::Happy:
            return "Happy";
        case CreatureMoodLevel::Neutral:
            return "Neutral";
        case CreatureMoodLevel::Upset:
            return "Upset";
        case CreatureMoodLevel::Angry:
            return "Angry";
        case CreatureMoodLevel::Furious:
            return "Furious";
        default:
            OD_LOG_ERR("moodLevel=" + Helper::toString(static_cast<int>(moodLevel)));
            return "";
    }
}
