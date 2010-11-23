#include "SoundEffectsHelper.h"

template<> SoundEffectsHelper*
	Ogre::Singleton<SoundEffectsHelper>::ms_Singleton = 0;

/*! \brief Returns a reference to the singleton object of SoundEffectsHelper.
 *
 */
SoundEffectsHelper& SoundEffectsHelper::getSingleton()
{
	assert(ms_Singleton);
	return (*ms_Singleton);
}

/*! \brief Returns a pointer to the singleton object of SoundEffectsHelper.
 *
 */
SoundEffectsHelper* SoundEffectsHelper::getSingletonPtr()
{
	return ms_Singleton;
}

/*! \brief Loads sounds
 *
 */
SoundEffectsHelper::SoundEffectsHelper()
	: soundManager(OgreOggSound::OgreOggSoundManager::getSingleton())
	, nextDigSound(0)
{
	// TODO Auto-generated constructor stub
	const Ogre::String digFolder = "RocksFalling/";
	digSounds.push_back(soundManager.createSound("dig1", digFolder + "Rocks\ Falling01.ogg"));
	digSounds.push_back(soundManager.createSound("dig2", digFolder + "Rocks\ Falling02.ogg"));
	digSounds.push_back(soundManager.createSound("dig3", digFolder + "Rocks\ Falling03.ogg"));
	digSounds.push_back(soundManager.createSound("dig4", digFolder + "Rocks\ Falling04.ogg"));
	digSounds.push_back(soundManager.createSound("dig5", digFolder + "Rocks\ Falling05.ogg"));
	digSounds.push_back(soundManager.createSound("dig6", digFolder + "Rocks\ Falling06.ogg"));
	digSounds.push_back(soundManager.createSound("dig7", digFolder + "Rocks\ Falling07.ogg"));
}

SoundEffectsHelper::~SoundEffectsHelper() {
	// TODO Auto-generated destructor stub
}

/*! \brief Playes sound for destroyed block at chosen position, and cycles through the different versions.
 *
 */
void SoundEffectsHelper::playBlockDestroySound(int tileX, int tileY)
{
	const float zPos = 1.5; //tile is from -0.25 to 3.0, floor is z0, so using the middle.
	//std::cout << "\n=========================================Playing rock fall sound at: " << tileX << " , " << tileY << std::endl;

	assert(digSounds[nextDigSound] != 0);
	if(digSounds[nextDigSound]->isPlaying())
	{
		digSounds[nextDigSound]->stop();

	}
	digSounds[nextDigSound]->setPosition(tileX, tileY, zPos);
	digSounds[nextDigSound]->play();
	++nextDigSound;
	if(nextDigSound >= digSounds.size())
	{
		nextDigSound = 0;
	}
}
