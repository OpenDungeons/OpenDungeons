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

#include "creaturemood/CreatureMoodManager.h"
#include "entities/Creature.h"

class CreatureMoodFactoryHunger : public CreatureMoodFactory
{
    CreatureMood* createCreatureMood() const override
    { return new CreatureMoodHunger; }

    const std::string& getCreatureMoodName() const override
    {
        static const std::string name = "Hunger";
        return name;
    }
};

//! \brief Register the mood type
static CreatureMoodRegister reg(new CreatureMoodFactoryHunger);

int32_t CreatureMoodHunger::computeMood(const Creature* creature) const
{
    int32_t hunger = static_cast<int32_t>(creature->getHunger());
    if(hunger < mStartHunger)
        return 0;

    return (hunger - mStartHunger) * mMoodModifier;
}

bool CreatureMoodHunger::importFromStream(std::istream& is)
{
    if(!CreatureMood::importFromStream(is))
        return false;

    if(!(is >> mStartHunger))
        return false;
    if(!(is >> mMoodModifier))
        return false;

    return true;
}
