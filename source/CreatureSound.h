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

#include "SoundEffectsManager.h"
#include "ODPacket.h"

//! \brief Class to store the sound sources for an individual creature and handle sound playback.
class CreatureSound
{
    friend class SoundEffectsManager;
public:

    // The various sound types used.
    enum SoundType
    {
        ATTACK = 0,
        DIGGING,
        PICKUP,
        DROP,
        IDLE,
        NUM_CREATURE_SOUNDS
    };

    //! \brief Play the wanted sound
    //! taken within the list of available sounds at random.
    void play(SoundType type);

    //! \brief Set the play position for the sound source.
    void setPosition(Ogre::Vector3 p);
    void setPosition(float x, float y, float z);

    friend ODPacket& operator<<(ODPacket& os, const SoundType& nt);
    friend ODPacket& operator>>(ODPacket& is, SoundType& nt);

private:
    //! \brief prevents unwanted creation or copy of the object.
    CreatureSound();
    CreatureSound(const CreatureSound&);
    CreatureSound& operator=(const CreatureSound&);

    //! \brief The list of available sound to play per type.
    //! \warning The GameSound objects are handled by the SoundEffectsManager class,
    //! do not delete them here.
    std::vector< std::vector<GameSound*> > mSoundsPerType;

    //! \brief Keeps the last index of sound played per type.
    //! This permit to force the variety in the sounds heard.
    std::vector<int> mLastSoundPlayedPerTypeId;

};

#endif // CREATURESOUND_H_
