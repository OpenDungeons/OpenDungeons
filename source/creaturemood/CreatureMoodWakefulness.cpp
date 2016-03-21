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

#include "creaturemood/CreatureMoodWakefulness.h"

#include "creaturemood/CreatureMoodManager.h"
#include "entities/Creature.h"

static const std::string CreatureMoodWakefulnessName = "Wakefulness";

namespace
{
class CreatureMoodWakefulnessFactory : public CreatureMoodFactory
{
    CreatureMood* createCreatureMood() const override
    { return new CreatureMoodWakefulness; }

    const std::string& getCreatureMoodName() const override
    {
        return CreatureMoodWakefulnessName;
    }
};

// Register the factory
static CreatureMoodRegister reg(new CreatureMoodWakefulnessFactory);
}

const std::string& CreatureMoodWakefulness::getModifierName() const
{
    return CreatureMoodWakefulnessName;
}

int32_t CreatureMoodWakefulness::computeMood(const Creature& creature) const
{
    int32_t wakefulness = static_cast<int32_t>(creature.getWakefulness());
    if(wakefulness > mStartWakefulness)
        return 0;

    return (mStartWakefulness - wakefulness) * mMoodModifier;
}

CreatureMoodWakefulness* CreatureMoodWakefulness::clone() const
{
    return new CreatureMoodWakefulness(*this);
}

bool CreatureMoodWakefulness::importFromStream(std::istream& is)
{
    if(!CreatureMood::importFromStream(is))
        return false;

    if(!(is >> mStartWakefulness))
        return false;
    if(!(is >> mMoodModifier))
        return false;

    return true;
}

void CreatureMoodWakefulness::exportToStream(std::ostream& os) const
{
    CreatureMood::exportToStream(os);
    os << "\t" << mStartWakefulness;
    os << "\t" << mMoodModifier;
}

void CreatureMoodWakefulness::getFormatString(std::string& format) const
{
    CreatureMood::getFormatString(format);
    if(!format.empty())
        format += "\t";

    format += "StartWakefulness\tMoodModifier";
}
