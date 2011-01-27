#ifndef SOUNDEFFECTSHELPER_H
#define SOUNDEFFECTSHELPER_H

#include <OgreSingleton.h>
#include <OgreQuaternion.h>
#include <OgreVector3.h>
//#include <OgreOggSoundManager.h>
//#include <OgreOggISound.h>
#include <SFML/Audio.hpp>
#include <vector>
#include <iostream>
#include <CEGUIMouseCursor.h>

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

	void initialiseSound(Ogre::String soundFolderPath);
	void setListenerPosition(const Ogre::Vector3& position, const Ogre::Quaternion& orientation);//, const Ogre::Vector3& velocity = Ogre::Vector3::ZERO);
	void playBlockDestroySound(int tileX, int tileY);
	void playInterfaceSound(InterfaceSound sound, bool stopCurrent = true);
private:
	//OgreOggSound::OgreOggSoundManager& soundManager;

	typedef std::vector<sf::Sound> SoundFXVector;
	typedef std::vector<sf::SoundBuffer> SoundFXBufferVector;

	//List of sounds for block getting dug out
	SoundFXVector digSounds;
	SoundFXBufferVector digSoundBuffers;
	//Next dig sound to be played
	unsigned nextDigSound;

	//Interface sounds, such as clicks
	SoundFXVector interfaceSounds;
	SoundFXBufferVector interfaceSoundBuffers;
};

#endif /* SOUNDEFFECTSHELPER_H */
