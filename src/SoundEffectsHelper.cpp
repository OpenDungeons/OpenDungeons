#include "SoundEffectsHelper.h"
#include <map>
#include "CreatureSound.h"

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
	: //soundManager(OgreOggSound::OgreOggSoundManager::getSingleton())
	 nextDigSound(0)
{


}

SoundEffectsHelper::~SoundEffectsHelper() {
	// TODO Auto-generated destructor stub
}

/*! \brief Initialise the sound system used.
 *
 */
void SoundEffectsHelper::initialiseSound(Ogre::String soundFolderPath)
{
    //std::cout << "Initialising sound system" << std::endl;
    //OgreOggSound::OgreOggSoundManager::getSingleton().init("", 100, 64, mSceneMgr);
    //assert(SoundEffectsHelper::getSingletonPtr() == 0);
    // Hardcoded for now
    const Ogre::String digFolder = soundFolderPath + "/RocksFalling/";
    digSoundBuffers.push_back(sf::SoundBuffer());
    digSoundBuffers.back().LoadFromFile(digFolder + "RocksFalling01.ogg");
    digSoundBuffers.push_back(sf::SoundBuffer());
    digSoundBuffers.back().LoadFromFile(digFolder + "RocksFalling02.ogg");
    digSoundBuffers.push_back(sf::SoundBuffer());
    digSoundBuffers.back().LoadFromFile(digFolder + "RocksFalling03.ogg");
    digSoundBuffers.push_back(sf::SoundBuffer());
    digSoundBuffers.back().LoadFromFile(digFolder + "RocksFalling04.ogg");
    digSoundBuffers.push_back(sf::SoundBuffer());
    digSoundBuffers.back().LoadFromFile(digFolder + "RocksFalling05.ogg");
    digSoundBuffers.push_back(sf::SoundBuffer());
    digSoundBuffers.back().LoadFromFile(digFolder + "RocksFalling06.ogg");
    digSoundBuffers.push_back(sf::SoundBuffer());
    digSoundBuffers.back().LoadFromFile(digFolder + "RocksFalling07.ogg");


    for(unsigned i = 0; i < digSoundBuffers.size(); ++i)
    {
        digSounds.push_back(sf::Sound());
        digSounds[i].SetBuffer(digSoundBuffers[i]);
    }

    interfaceSoundBuffers.push_back(sf::SoundBuffer());
    interfaceSoundBuffers.back().LoadFromFile(soundFolderPath + "/Click/click.ogg");



    //Replacement sound for now
    while(interfaceSounds.size() < NUM_INTERFACE_SOUNDS)
    {
        interfaceSounds.push_back(sf::Sound(interfaceSoundBuffers.back()));
        //These sounds are not positioned, this disables
        //Spatialisation for the sound.
        interfaceSounds.back().SetAttenuation(0);
    }


    creatureSoundBuffers["Default"] = SoundFXBufferVector();
    SoundFXBufferVector& buffers = creatureSoundBuffers["Default"];
    buffers.assign(CreatureSound::NUM_CREATURE_SOUNDS, sf::SoundBuffer());
    buffers[CreatureSound::ATTACK].LoadFromFile(soundFolderPath + "/Sword/SwordBlock01.ogg");
    buffers[CreatureSound::DIG].LoadFromFile(soundFolderPath + "/Digging/Digging01.ogg");
    //buffers[CreatureSound::DROP].LoadFromFile(soundFolderPath + "/Click/click.ogg);
}

void SoundEffectsHelper::setListenerPosition(const Ogre::Vector3& position, const Ogre::Quaternion& orientation)//, const Ogre::Vector3& velocity)
{
    sf::Listener::SetPosition(static_cast<float>(position.x),
            static_cast<float>(position.y),
            static_cast<float>(position.z));

    //TODO - verify if this is right
    Ogre::Vector3 vDir = orientation.zAxis();
    sf::Listener::SetTarget(-vDir.x, -vDir.y, -vDir.z);
}

/*! \brief Playes sound for destroyed block at chosen position, and cycles through the different versions.
 *
 */
void SoundEffectsHelper::playBlockDestroySound(int tileX, int tileY)
{
	const float zPos = 1.5; //tile is from -0.25 to 3.0, floor is z0, so using the middle.
	//std::cout << "\n=========================================Playing rock fall sound at: " << tileX << " , " << tileY << std::endl;

	assert(digSounds.size() > 0);
	if(digSounds[nextDigSound].Playing)
	{
		digSounds[nextDigSound].Stop();

	}
	digSounds[nextDigSound].SetPosition(tileX, tileY, zPos);
	digSounds[nextDigSound].Play();
	++nextDigSound;
	if(nextDigSound >= digSounds.size())
	{
		nextDigSound = 0;
	}
}

void SoundEffectsHelper::playInterfaceSound(InterfaceSound sound, bool stopCurrent)
{
    if(stopCurrent)
    {
        interfaceSounds[sound].Stop();
    }

    interfaceSounds[sound].Play();
}

void SoundEffectsHelper::registerCreatureClass(const std::string& creatureClass)
{
    //creatureSoundBuffers.insert(creatureClass.)
    //Not implemented yet
}

Ogre::SharedPtr<CreatureSound> SoundEffectsHelper::createCreatureSound(const std::string& creatureClass)
{
    Ogre::SharedPtr<CreatureSound> sound(new CreatureSound());
    SoundFXVector& soundVector = sound->sounds;
    SoundFXBufferVector& buffers = creatureSoundBuffers["Default"];
    soundVector[CreatureSound::ATTACK].SetBuffer(buffers[CreatureSound::ATTACK]);
    soundVector[CreatureSound::DIG].SetBuffer(buffers[CreatureSound::DIG]);
    return sound;
}
