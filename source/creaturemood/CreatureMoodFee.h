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

#ifndef CREATUREMOODFEE_H
#define CREATUREMOODFEE_H

#include "creaturemood/CreatureMood.h"

class CreatureMoodFee : public CreatureMood
{
public:
    CreatureMoodFee(int32_t moodModifier) :
        mMoodModifier(moodModifier)
    {}

    virtual ~CreatureMoodFee() {}

    virtual CreatureMoodType getCreatureMoodType() const
    { return CreatureMoodType::fee; }

    virtual int32_t computeMood(const Creature* creature) const;

    inline CreatureMood* clone() const
    {  return new CreatureMoodFee(mMoodModifier); }

private:
    int32_t mMoodModifier;
};

#endif // CREATUREMOODFEE_H
