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
            OD_ASSERT_TRUE_MSG(false, "moodLevel=" + Helper::toString(static_cast<int>(moodLevel)));
            return "";
    }
}

CreatureMood* CreatureMood::load(std::istream& defFile)
{
    std::string nextParam;
    CreatureMood* def = nullptr;
    while (defFile.good())
    {
        if(!(defFile >> nextParam))
            break;

        if (nextParam == "[/MoodModifiers]" || nextParam == "[/MoodModifier]")
            return def;

        if(def != nullptr)
        {
            // The previous line was a valid def so we should have had an ending tag
            OD_ASSERT_TRUE_MSG(false, "nextParam=" + nextParam);
            return def;
        }

        if (nextParam == "Awakness")
        {
            if(!(defFile >> nextParam))
                break;
            int32_t startAwakness = Helper::toInt(nextParam);
            if(!(defFile >> nextParam))
                break;
            int32_t moodModifier = Helper::toInt(nextParam);

            def = new CreatureMoodAwakness(startAwakness, moodModifier);
        }

        if (nextParam == "Creature")
        {
            std::string creatureClass;
            if(!(defFile >> creatureClass))
                break;
            if(!(defFile >> nextParam))
                break;
            int32_t moodModifier = Helper::toInt(nextParam);

            def = new CreatureMoodCreature(creatureClass, moodModifier);
        }

        if (nextParam == "Fee")
        {
            if(!(defFile >> nextParam))
                break;
            int32_t moodModifier = Helper::toInt(nextParam);

            def = new CreatureMoodFee(moodModifier);
        }

        if (nextParam == "HpLoss")
        {
            if(!(defFile >> nextParam))
                break;
            int32_t moodModifier = Helper::toInt(nextParam);

            def = new CreatureMoodHpLoss(moodModifier);
        }

        if (nextParam == "Hunger")
        {
            if(!(defFile >> nextParam))
                break;
            int32_t startHunger = Helper::toInt(nextParam);
            if(!(defFile >> nextParam))
                break;
            int32_t moodModifier = Helper::toInt(nextParam);

            def = new CreatureMoodHunger(startHunger, moodModifier);
        }

        if (nextParam == "TurnsWithoutFight")
        {
            if(!(defFile >> nextParam))
                break;
            int32_t nbTurnsWithoutFightMin = Helper::toInt(nextParam);
            if(!(defFile >> nextParam))
                break;
            int32_t nbTurnsWithoutFightMax = Helper::toInt(nextParam);
            if(!(defFile >> nextParam))
                break;
            int32_t moodModifier = Helper::toInt(nextParam);

            def = new CreatureMoodTurnsWithoutFight(nbTurnsWithoutFightMin,
                nbTurnsWithoutFightMax, moodModifier);
        }
    }
    OD_ASSERT_TRUE(false);
    return nullptr;
}
