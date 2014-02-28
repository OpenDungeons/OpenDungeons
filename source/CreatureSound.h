/*
 *  Created on: 24. feb. 2011
 *  Author: oln
 *
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#ifndef CREATURESOUND_H_
#define CREATURESOUND_H_

#include "SoundEffectsHelper.h"

//! \brief Class to store the sound sources for an individual creature and handle sound playback.
class CreatureSound
{
    friend class SoundEffectsHelper;
public:

    // The various sound types used.
    enum SoundType
    {
        ATTACK,
        DIG,
        //DROP,
        //IDLE1,
        NUM_CREATURE_SOUNDS
    };

    void play(SoundType type);
    void playDelayed(SoundType type);
    void setPosition(Ogre::Vector3 p);
    void setPosition(float x, float y, float z);
private:
    CreatureSound();
    CreatureSound(const CreatureSound&);
    CreatureSound& operator=(const CreatureSound&);

    SoundEffectsHelper::SoundFXVector mSounds;
};

#endif // CREATURESOUND_H_
