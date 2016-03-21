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

#include "creaturemood/CreatureMoodHpLoss.h"

#include "creaturemood/CreatureMoodManager.h"
#include "entities/Creature.h"
#include "utils/Helper.h"

static const std::string CreatureMoodHpLossName = "HpLoss";

namespace
{
class CreatureMoodHpLossFactory : public CreatureMoodFactory
{
    CreatureMood* createCreatureMood() const override
    { return new CreatureMoodHpLoss; }

    const std::string& getCreatureMoodName() const override
    {
        return CreatureMoodHpLossName;
    }
};

// Register the factory
static CreatureMoodRegister reg(new CreatureMoodHpLossFactory);
}

const std::string& CreatureMoodHpLoss::getModifierName() const
{
    return CreatureMoodHpLossName;
}

int32_t CreatureMoodHpLoss::computeMood(const Creature& creature) const
{
    int32_t hpLost = creature.getMaxHp() - creature.getHP();
    if(hpLost <= 0)
        return 0;

    hpLost = Helper::round(static_cast<float>(hpLost));
    return hpLost * mMoodModifier;
}

CreatureMoodHpLoss* CreatureMoodHpLoss::clone() const
{
    return new CreatureMoodHpLoss(*this);
}

bool CreatureMoodHpLoss::importFromStream(std::istream& is)
{
    if(!CreatureMood::importFromStream(is))
        return false;

    if(!(is >> mMoodModifier))
        return false;

    return true;
}

void CreatureMoodHpLoss::exportToStream(std::ostream& os) const
{
    CreatureMood::exportToStream(os);
    os << "\t" << mMoodModifier;
}

void CreatureMoodHpLoss::getFormatString(std::string& format) const
{
    CreatureMood::getFormatString(format);
    if(!format.empty())
        format += "\t";

    format += "MoodModifier";
}
