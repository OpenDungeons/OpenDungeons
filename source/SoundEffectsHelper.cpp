#include <map>

#include "CreatureSound.h"
#include "ResourceManager.h"

#include "SoundEffectsHelper.h"

//Z value to use for tile positioned sounds
const float TILE_ZPOS = 2.5;

template<> SoundEffectsHelper*
        Ogre::Singleton<SoundEffectsHelper>::msSingleton = 0;

/*! \brief Loads sounds
 *
 */
SoundEffectsHelper::SoundEffectsHelper() :
        nextDigSound(0)
{
    //TODO: put all hardcoded stuff in a general place, like ResourceManager
    Ogre::String soundFolderPath = ResourceManager::getSingletonPtr()->getSoundPath();
    Ogre::String digFolder = soundFolderPath + "RocksFalling/";

    for (int i = 0; i < 7; ++i)
    {
        digSoundBuffers.push_back(Ogre::SharedPtr<sf::SoundBuffer>(
                new sf::SoundBuffer()));
    }
    //digSoundBuffers.assign(7, sf::SoundBuffer());
    digSoundBuffers[0]->LoadFromFile(digFolder + "RocksFalling01.ogg");
    digSoundBuffers[1]->LoadFromFile(digFolder + "RocksFalling02.ogg");
    digSoundBuffers[2]->LoadFromFile(digFolder + "RocksFalling03.ogg");
    digSoundBuffers[3]->LoadFromFile(digFolder + "RocksFalling04.ogg");
    digSoundBuffers[4]->LoadFromFile(digFolder + "RocksFalling05.ogg");
    digSoundBuffers[5]->LoadFromFile(digFolder + "RocksFalling06.ogg");
    digSoundBuffers[6]->LoadFromFile(digFolder + "RocksFalling07.ogg");

    for (unsigned i = 0; i < digSoundBuffers.size(); ++i)
    {
        digSounds.push_back(sf::Sound());
        digSounds[i].SetBuffer(*digSoundBuffers[i].get());
    }

    for (int i = 0; i < NUM_INTERFACE_SOUNDS; ++i)
    {
        interfaceSoundBuffers.push_back(Ogre::SharedPtr<sf::SoundBuffer>(
                new sf::SoundBuffer()));
    }

    interfaceSoundBuffers[SoundEffectsHelper::BUTTONCLICK]->LoadFromFile(
            soundFolderPath + "Click/click.ogg");
    interfaceSoundBuffers[SoundEffectsHelper::DIGSELECT]->LoadFromFile(
            soundFolderPath + "Click/click.ogg");
    interfaceSoundBuffers[SoundEffectsHelper::PICKUP]->LoadFromFile(soundFolderPath
            + "Click/click.ogg");
    interfaceSoundBuffers[SoundEffectsHelper::DROP]->LoadFromFile(soundFolderPath
            + "Click/click.ogg");
    interfaceSoundBuffers[SoundEffectsHelper::BUILDROOM]->LoadFromFile(
            soundFolderPath + "RoomBuild/bump.ogg");
    interfaceSoundBuffers[SoundEffectsHelper::BUILDTRAP]->LoadFromFile(
            soundFolderPath + "RoomBuild/bump.ogg");
    interfaceSoundBuffers[SoundEffectsHelper::CLAIM]->LoadFromFile(soundFolderPath
            + "ClaimTile/Claim01.ogg");

    //Replacement sound for now
    for (int i = 0; i < NUM_INTERFACE_SOUNDS; ++i)
    {
        interfaceSounds.push_back(sf::Sound(*interfaceSoundBuffers[i]));
    }
    //Disable spatialisation
    //TODO - some of these should be positioned
    interfaceSounds[SoundEffectsHelper::BUTTONCLICK].SetAttenuation(0);
    interfaceSounds[SoundEffectsHelper::DIGSELECT].SetAttenuation(0);
    interfaceSounds[SoundEffectsHelper::PICKUP].SetAttenuation(0);
    interfaceSounds[SoundEffectsHelper::DROP].SetAttenuation(0);
    interfaceSounds[SoundEffectsHelper::BUILDROOM].SetAttenuation(0);
    interfaceSounds[SoundEffectsHelper::BUILDTRAP].SetAttenuation(0);

    interfaceSounds[SoundEffectsHelper::BUTTONCLICK].SetVolume(25.0f);
    interfaceSounds[SoundEffectsHelper::DIGSELECT].SetVolume(25.0f);
    interfaceSounds[SoundEffectsHelper::PICKUP].SetVolume(25.0f);
    interfaceSounds[SoundEffectsHelper::DROP].SetVolume(25.0f);
    interfaceSounds[SoundEffectsHelper::BUILDROOM].SetVolume(25.0f);
    interfaceSounds[SoundEffectsHelper::BUILDTRAP].SetVolume(25.0f);

    creatureSoundBuffers["Default"] = SoundFXBufferVector();
    SoundFXBufferVector& buffers = creatureSoundBuffers["Default"];
    for (int i = 0; i < CreatureSound::NUM_CREATURE_SOUNDS; ++i)
    {
        buffers.push_back(Ogre::SharedPtr<sf::SoundBuffer>(
                new sf::SoundBuffer()));
    }
    buffers[CreatureSound::ATTACK]->LoadFromFile(soundFolderPath
            + "Sword/SwordBlock01.ogg");
    buffers[CreatureSound::DIG]->LoadFromFile(soundFolderPath
            + "Digging/Digging01.ogg");
    //buffers[CreatureSound::DROP].LoadFromFile(soundFolderPath + "/Click/click.ogg);
}

SoundEffectsHelper::~SoundEffectsHelper()
{
    // TODO Auto-generated destructor stub
}

void SoundEffectsHelper::setListenerPosition(const Ogre::Vector3& position,
        const Ogre::Quaternion& orientation)
{
    sf::Listener::SetPosition(static_cast<float> (position.x),
            static_cast<float> (position.y), static_cast<float> (position.z));

    //TODO - verify if this is right
    Ogre::Vector3 vDir = orientation.zAxis();
    sf::Listener::SetTarget(-vDir.x, -vDir.y, -vDir.z);
}

/*! \brief Playes sound for destroyed block at chosen position, and cycles through the different versions.
 *
 */
void SoundEffectsHelper::playBlockDestroySound(int tileX, int tileY)
{
    //tile is from -0.25 to 3.0, floor is z0, so using the middle.
    //std::cout << "\n=========================================Playing rock fall sound at: " << tileX << " , " << tileY << std::endl;

    assert(!digSounds.empty());
    if (digSounds[nextDigSound].Playing)
    {
        digSounds[nextDigSound].Stop();

    }
    digSounds[nextDigSound].SetPosition(tileX, tileY, TILE_ZPOS);
    digSounds[nextDigSound].Play();

    if(++nextDigSound >= digSounds.size())
    {
        nextDigSound = 0;
    }
}

void SoundEffectsHelper::playInterfaceSound(InterfaceSound sound,
        bool stopCurrent)
{
    if (stopCurrent)
    {
        interfaceSounds[sound].Stop();
    }

    interfaceSounds[sound].Play();
}

void SoundEffectsHelper::playInterfaceSound(InterfaceSound sound,
        const Ogre::Vector3& position, bool stopCurrent)
{
    interfaceSounds[sound].SetPosition(static_cast<float> (position.x),
            static_cast<float> (position.y), static_cast<float> (position.z));
    playInterfaceSound(sound, stopCurrent);
}

void SoundEffectsHelper::playInterfaceSound(InterfaceSound sound, int tileX,
        int tileY, bool stopCurrent)
{
    interfaceSounds[sound].SetPosition(static_cast<float> (tileX),
            static_cast<float> (tileY), TILE_ZPOS);
    playInterfaceSound(sound, stopCurrent);
}

void SoundEffectsHelper::registerCreatureClass(const std::string& creatureClass)
{
    //creatureSoundBuffers.insert(creatureClass.)
    //Not implemented yet
}

Ogre::SharedPtr<CreatureSound> SoundEffectsHelper::createCreatureSound(
        const std::string& creatureClass)
{
    Ogre::SharedPtr<CreatureSound> sound(new CreatureSound());
    SoundFXVector& soundVector = sound->sounds;
    SoundFXBufferVector& buffers = creatureSoundBuffers["Default"];
    soundVector[CreatureSound::ATTACK].SetBuffer(
            *buffers[CreatureSound::ATTACK].get());
    soundVector[CreatureSound::DIG].SetBuffer(
            *buffers[CreatureSound::DIG].get());
    return sound;
}
