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
    digSoundBuffers[0]->loadFromFile(digFolder + "RocksFalling01.ogg");
    digSoundBuffers[1]->loadFromFile(digFolder + "RocksFalling02.ogg");
    digSoundBuffers[2]->loadFromFile(digFolder + "RocksFalling03.ogg");
    digSoundBuffers[3]->loadFromFile(digFolder + "RocksFalling04.ogg");
    digSoundBuffers[4]->loadFromFile(digFolder + "RocksFalling05.ogg");
    digSoundBuffers[5]->loadFromFile(digFolder + "RocksFalling06.ogg");
    digSoundBuffers[6]->loadFromFile(digFolder + "RocksFalling07.ogg");

    for (unsigned i = 0; i < digSoundBuffers.size(); ++i)
    {
        digSounds.push_back(sf::Sound());
        digSounds[i].setBuffer(*digSoundBuffers[i].get());
    }

    for (int i = 0; i < NUM_INTERFACE_SOUNDS; ++i)
    {
        interfaceSoundBuffers.push_back(Ogre::SharedPtr<sf::SoundBuffer>(
                new sf::SoundBuffer()));
    }

    interfaceSoundBuffers[SoundEffectsHelper::BUTTONCLICK]->loadFromFile(
            soundFolderPath + "Click/click.ogg");
    interfaceSoundBuffers[SoundEffectsHelper::DIGSELECT]->loadFromFile(
            soundFolderPath + "Click/click.ogg");
    interfaceSoundBuffers[SoundEffectsHelper::PICKUP]->loadFromFile(soundFolderPath
            + "Click/click.ogg");
    interfaceSoundBuffers[SoundEffectsHelper::DROP]->loadFromFile(soundFolderPath
            + "Click/click.ogg");
    interfaceSoundBuffers[SoundEffectsHelper::BUILDROOM]->loadFromFile(
            soundFolderPath + "RoomBuild/bump.ogg");
    interfaceSoundBuffers[SoundEffectsHelper::BUILDTRAP]->loadFromFile(
            soundFolderPath + "RoomBuild/bump.ogg");
    interfaceSoundBuffers[SoundEffectsHelper::CLAIM]->loadFromFile(soundFolderPath
            + "ClaimTile/Claim01.ogg");

    //Replacement sound for now
    for (int i = 0; i < NUM_INTERFACE_SOUNDS; ++i)
    {
        interfaceSounds.push_back(sf::Sound(*interfaceSoundBuffers[i]));
    }
    //Disable spatialisation
    //TODO - some of these should be positioned
    interfaceSounds[SoundEffectsHelper::BUTTONCLICK].setAttenuation(0);
    interfaceSounds[SoundEffectsHelper::DIGSELECT].setAttenuation(0);
    interfaceSounds[SoundEffectsHelper::PICKUP].setAttenuation(0);
    interfaceSounds[SoundEffectsHelper::DROP].setAttenuation(0);
    interfaceSounds[SoundEffectsHelper::BUILDROOM].setAttenuation(0);
    interfaceSounds[SoundEffectsHelper::BUILDTRAP].setAttenuation(0);

    interfaceSounds[SoundEffectsHelper::BUTTONCLICK].setVolume(25.0f);
    interfaceSounds[SoundEffectsHelper::DIGSELECT].setVolume(25.0f);
    interfaceSounds[SoundEffectsHelper::PICKUP].setVolume(25.0f);
    interfaceSounds[SoundEffectsHelper::DROP].setVolume(25.0f);
    interfaceSounds[SoundEffectsHelper::BUILDROOM].setVolume(25.0f);
    interfaceSounds[SoundEffectsHelper::BUILDTRAP].setVolume(25.0f);

    creatureSoundBuffers["Default"] = SoundFXBufferVector();
    SoundFXBufferVector& buffers = creatureSoundBuffers["Default"];
    for (int i = 0; i < CreatureSound::NUM_CREATURE_SOUNDS; ++i)
    {
        buffers.push_back(Ogre::SharedPtr<sf::SoundBuffer>(
                new sf::SoundBuffer()));
    }
    buffers[CreatureSound::ATTACK]->loadFromFile(soundFolderPath
            + "Sword/SwordBlock01.ogg");
    buffers[CreatureSound::DIG]->loadFromFile(soundFolderPath
            + "Digging/Digging01.ogg");
    //buffers[CreatureSound::DROP].loadFromFile(soundFolderPath + "/Click/click.ogg);
}

SoundEffectsHelper::~SoundEffectsHelper()
{
    // TODO Auto-generated destructor stub
}

void SoundEffectsHelper::setListenerPosition(const Ogre::Vector3& position,
        const Ogre::Quaternion& orientation)
{
    sf::Listener::setPosition(static_cast<float> (position.x),
            static_cast<float> (position.y), static_cast<float> (position.z));

    //TODO - verify if this is right
    Ogre::Vector3 vDir = orientation.zAxis();
    //FIXME Are those really sets the orientation of players ears 
    //Acording to SFML docs this should be a point in ABSOLUTE coordinates
    //Where player is targeting his ear :P . paul424 .
    sf::Listener::setDirection(-vDir.x, -vDir.y, -vDir.z);
}

/*! \brief Playes sound for destroyed block at chosen position, and cycles through the different versions.
 *
 */
void SoundEffectsHelper::playBlockDestroySound(int tileX, int tileY)
{
    //tile is from -0.25 to 3.0, floor is z0, so using the middle.
    //std::cout << "\n=========================================Playing rock fall sound at: " << tileX << " , " << tileY << std::endl;

    assert(!digSounds.empty());
    if (digSounds[nextDigSound].getStatus() == sf::Sound::Playing)
    {
        digSounds[nextDigSound].stop();

    }
    digSounds[nextDigSound].setPosition(tileX, tileY, TILE_ZPOS);
    digSounds[nextDigSound].play();

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
        interfaceSounds[sound].stop();
    }

    interfaceSounds[sound].play();
}

void SoundEffectsHelper::playInterfaceSound(InterfaceSound sound,
        const Ogre::Vector3& position, bool stopCurrent)
{
    interfaceSounds[sound].setPosition(static_cast<float> (position.x),
            static_cast<float> (position.y), static_cast<float> (position.z));
    playInterfaceSound(sound, stopCurrent);
}

void SoundEffectsHelper::playInterfaceSound(InterfaceSound sound, int tileX,
        int tileY, bool stopCurrent)
{
    interfaceSounds[sound].setPosition(static_cast<float> (tileX),
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
    soundVector[CreatureSound::ATTACK].setBuffer(
            *buffers[CreatureSound::ATTACK].get());
    soundVector[CreatureSound::DIG].setBuffer(
            *buffers[CreatureSound::DIG].get());
    return sound;
}
