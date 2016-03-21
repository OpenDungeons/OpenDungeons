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

#include "creaturemood/CreatureMoodFee.h"

#include "creaturemood/CreatureMoodManager.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "utils/Helper.h"

static const std::string CreatureMoodFeeName = "Fee";

namespace
{
class CreatureMoodFeeFactory : public CreatureMoodFactory
{
    CreatureMood* createCreatureMood() const override
    { return new CreatureMoodFee; }

    const std::string& getCreatureMoodName() const override
    {
        return CreatureMoodFeeName;
    }
};

// Register the factory
static CreatureMoodRegister reg(new CreatureMoodFeeFactory);
}

const std::string& CreatureMoodFee::getModifierName() const
{
    return CreatureMoodFeeName;
}

int32_t CreatureMoodFee::computeMood(const Creature& creature) const
{
    int32_t owedGold = creature.getGoldFee() - creature.getDefinition()->getFee(creature.getLevel());
    if(owedGold < 100)
        return 0;

    owedGold = Helper::round(static_cast<double>(owedGold) * 0.01);
    return owedGold * mMoodModifier;
}

CreatureMoodFee* CreatureMoodFee::clone() const
{
    return new CreatureMoodFee(*this);
}

bool CreatureMoodFee::importFromStream(std::istream& is)
{
    if(!CreatureMood::importFromStream(is))
        return false;

    if(!(is >> mMoodModifier))
        return false;

    return true;
}

void CreatureMoodFee::exportToStream(std::ostream& os) const
{
    CreatureMood::exportToStream(os);
    os << "\t" << mMoodModifier;
}

void CreatureMoodFee::getFormatString(std::string& format) const
{
    CreatureMood::getFormatString(format);
    if(!format.empty())
        format += "\t";

    format += "MoodModifier";
}
