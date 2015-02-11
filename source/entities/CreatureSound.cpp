/*
 *  CreatureSound.cpp
 *
 *  Created on: 24. feb. 2011
 *  Author: oln, hwoarangmy, Bertram
 *
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

#include "entities/CreatureSound.h"

#include "network/ODPacket.h"
#include "utils/Random.h"

CreatureSound::CreatureSound()
{
    // Reserve space for sound objects
    for (unsigned int i = 0; i < NUM_CREATURE_SOUNDS; ++i)
    {
        mSoundsPerType.push_back(std::vector<GameSound*>());
        // Init the last played sound with invalid values.
        mLastSoundPlayedPerTypeId.push_back(-1);
    }
}

void CreatureSound::play(SoundType type, float x, float y, float z)
{
    std::vector<GameSound*>& soundList = mSoundsPerType[type];
    if (soundList.empty())
        return;

    int newSoundIdPlayed = Random::Int(0, soundList.size() - 1);

    // We set a new sound index value at random if possible.
    if (soundList.size() > 1)
    {
        while(newSoundIdPlayed == mLastSoundPlayedPerTypeId[type])
            newSoundIdPlayed = Random::Int(0, soundList.size() - 1);
    }

    // Then play the new sound
    soundList[newSoundIdPlayed]->play(x, y, z);
    mLastSoundPlayedPerTypeId[type] = newSoundIdPlayed;
}


ODPacket& operator<<(ODPacket& os, const CreatureSound::SoundType& nt)
{
    os << static_cast<int32_t>(nt);
    return os;
}

ODPacket& operator>>(ODPacket& is, CreatureSound::SoundType& nt)
{
    int32_t tmp;
    is >> tmp;
    nt = static_cast<CreatureSound::SoundType>(tmp);
    return is;
}
