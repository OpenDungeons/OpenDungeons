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
	SoundEffectsHelper();
	virtual ~SoundEffectsHelper();
	static SoundEffectsHelper& getSingleton();
	static SoundEffectsHelper* getSingletonPtr();

	void playBlockDestroySound(int tileX, int tileY);
private:
	OgreOggSound::OgreOggSoundManager& soundManager;

	typedef std::vector<OgreOggSound::OgreOggISound*> SoundFXVector;

	//List of sounds for block getting dug out
	SoundFXVector digSounds;
	//Next dig sound to be played
	unsigned nextDigSound;
};

#endif /* SOUNDEFFECTSHELPER_H */
