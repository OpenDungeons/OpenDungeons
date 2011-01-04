#ifndef SOUNDEFFECTSHELPER_H
#define SOUNDEFFECTSHELPER_H

#include <OgreSingleton.h>
#include <OgreOggSoundManager.h>
#include <OgreOggISound.h>
#include <vector>
#include <iostream>

/*! \brief Helper class to manage sound effects.
 *
 */
class SoundEffectsHelper : public Ogre::Singleton<SoundEffectsHelper>
{
public:
    enum InterfaceSound {
        BUTTONCLICK,
        DIGSELECT,
        PICKUP,
        //NOTE Pickup and Drop will be added to individual creatures later
        //, though may still be used for items or something
        DROP,
        BUILDROOM,
        BUILDTRAP,
        NUM_INTERFACE_SOUNDS
    };

	SoundEffectsHelper();
	virtual ~SoundEffectsHelper();
	static SoundEffectsHelper& getSingleton();
	static SoundEffectsHelper* getSingletonPtr();

	void playBlockDestroySound(int tileX, int tileY);
	void playInterfaceSound(InterfaceSound sound, bool stopCurrent = true);
private:
	OgreOggSound::OgreOggSoundManager& soundManager;

	typedef std::vector<OgreOggSound::OgreOggISound*> SoundFXVector;

	//List of sounds for block getting dug out
	SoundFXVector digSounds;
	//Next dig sound to be played
	unsigned nextDigSound;

	//Interface sounds, such as clicks
	SoundFXVector interfaceSounds;
};

#endif /* SOUNDEFFECTSHELPER_H */
