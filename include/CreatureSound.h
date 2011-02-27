/*
 * CreatureSound.h
 *
 *  Created on: 24. feb. 2011
 *      Author: oln
 */

#ifndef CREATURESOUND_H_
#define CREATURESOUND_H_
#include "SoundEffectsHelper.h"

/*! \brief Class to store the sound sources for an individual creature and handle sound playback.
 *
 */
class CreatureSound
{

    friend class SoundEffectsHelper;
    public:

    //The various sound types used.
    //Extend as needed.
    enum SoundType {
        ATTACK,
        //ISHIT,
        DIG,
        //DROP,
        //IDLE1,
        NUM_CREATURE_SOUNDS
    };


    void play(SoundType type);
    void playDelayed(SoundType type);
    void setPosition(float x, float y, float z);
    private:
    CreatureSound();
    CreatureSound(const CreatureSound&);
    CreatureSound& operator=(const CreatureSound&);

    SoundEffectsHelper* sfHelper;
    //TODO - should we use a vector of pointers instead?
    SoundEffectsHelper::SoundFXVector sounds;
};

#endif /* CREATURESOUND_H_ */
