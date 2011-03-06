/*
 * CreatureSound.cpp
 *
 *  Created on: 24. feb. 2011
 *      Author: oln
 */

#include "CreatureSound.h"

CreatureSound::CreatureSound()
{
    //Reserve space for sound objects
    sounds.assign(NUM_CREATURE_SOUNDS, sf::Sound());
}

/*! \brief Play the wanted sound.
 *
 */
void CreatureSound::play(SoundType type)
{
    //TODO - can it ever happen that we start playing the same sound before it's finished?
    sounds[type].Stop();
    sounds[type].Play();
}

void CreatureSound::setPosition(Ogre::Vector3 p)
{
	setPosition(p.x, p.y, p.z);
}

/*! \brief Set the play position for the sound source.
 *
 */
void CreatureSound::setPosition(float x, float y, float z)
{
    //TODO - can this be optimised?
    SoundEffectsHelper::SoundFXVector::iterator it;
    for(it = sounds.begin(); it != sounds.end(); ++it)
    {
        it->SetPosition(x, y, z);
    }
}

