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

#include "creaturemood/CreatureMoodTurnsWithoutFight.h"

#include "creaturemood/CreatureMoodManager.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "utils/Helper.h"

class CreatureMoodTurnsWithoutFightFactory : public CreatureMoodFactory
{
    CreatureMood* createCreatureMood() const override
    { return new CreatureMoodTurnsWithoutFight; }

    const std::string& getCreatureMoodName() const override
    {
        static const std::string name = "TurnsWithoutFight";
        return name;
    }
};

//! \brief Register the mood type
static CreatureMoodRegister reg(new CreatureMoodTurnsWithoutFightFactory);

int32_t CreatureMoodTurnsWithoutFight::computeMood(const Creature* creature) const
{
    int32_t turns = creature->getNbTurnsWithoutBattle();
    if(turns < mTurnsWithoutFightMin)
        return 0;

    turns = std::min(turns - mTurnsWithoutFightMin, mTurnsWithoutFightMax);
    return turns * mMoodModifier;
}

bool CreatureMoodTurnsWithoutFight::importFromStream(std::istream& is)
{
    if(!CreatureMood::importFromStream(is))
        return false;

    if(!(is >> mTurnsWithoutFightMin))
        return false;
    if(!(is >> mTurnsWithoutFightMax))
        return false;
    if(!(is >> mMoodModifier))
        return false;

    return true;
}
