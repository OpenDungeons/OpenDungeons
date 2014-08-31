/*
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

#include "SoundEffectsManager.h"

#include "CreatureSound.h"
#include "ResourceManager.h"
#include "Random.h"

#include "boost/filesystem.hpp"

#include <map>

// The Z value to use for tile positioned sounds.
// Tiles are from -0.25 to 3.0 in z value, the floor is at 0,
// and we're using an pseudo-average value.
const float TILE_ZPOS = 2.5;

// class GameSound
GameSound::GameSound(const std::string& filename, bool spatialSound)
{
    // Loads the buffer
    mSoundBuffer = new sf::SoundBuffer();
    if (mSoundBuffer->loadFromFile(filename) == false)
    {
        delete mSoundBuffer;
        mSoundBuffer = NULL;
        return;
    }

    mFilename = filename;

    // Loads the main sound object
    mSound.setBuffer(*mSoundBuffer);

    // Sets a correct attenuation value if the sound is spatial
    if (spatialSound == true)
    {
        // Set convenient spatial fading unit.
        mSound.setVolume(100.0f);
        mSound.setAttenuation(3.0f);
        mSound.setMinDistance(3.0f);
    }
    else // Disable attenuation for sounds that must heard the same way everywhere
    {
        // Prevents the sound from being too loud
        mSound.setVolume(30.0f);
        mSound.setAttenuation(0.0f);
    }
}

GameSound::~GameSound()
{
    if (mSoundBuffer != NULL)
        delete mSoundBuffer;
}

void GameSound::setPosition(float x, float y, float z)
{
    mSound.setPosition(x, y, z);
}

// SoundEffectsManager class
template<> SoundEffectsManager* Ogre::Singleton<SoundEffectsManager>::msSingleton = 0;

SoundEffectsManager::SoundEffectsManager()
{
    initializeInterfaceSounds();
    initializeDefaultCreatureSounds();
}

SoundEffectsManager::~SoundEffectsManager()
{
    // Clear up every cached sounds...
    std::map<std::string, GameSound*>::iterator it = mGameSoundCache.begin();
    std::map<std::string, GameSound*>::iterator it_end = mGameSoundCache.begin();
    for (; it != it_end; ++it)
    {
        if (it->second != NULL)
            delete it->second;
    }

    std::map<std::string, CreatureSound*>::iterator it2 = mCreatureSoundCache.begin();
    std::map<std::string, CreatureSound*>::iterator it2_end = mCreatureSoundCache.begin();
    for (; it2 != it2_end; ++it2)
    {
        if (it2->second != NULL)
            delete it2->second;
    }

    // Delete the independent caches
    for (unsigned int i = 0; i < mClaimedSounds.size(); ++i)
        delete mClaimedSounds[i];
    for (unsigned int i = 0; i < mRocksFallSounds.size(); ++i)
        delete mRocksFallSounds[i];
    for (unsigned int i = 0; i < mInterfaceSounds.size(); ++i)
        delete mInterfaceSounds[i];
}

//! \brief blunt copy of the menu code listing files in a directory adapted to custom needs
//! \TODO: Might want to push all that in a common Helper function.
static bool fillFilesList(const std::string& path, std::vector<std::string>& listFiles,
                          const std::string& fileExtension)
{
    const boost::filesystem::path dir_path(path);
    if (!boost::filesystem::exists(dir_path))
        return false;
    boost::filesystem::directory_iterator end_itr;
    for (boost::filesystem::directory_iterator itr(dir_path); itr != end_itr; ++itr)
    {
        if(!boost::filesystem::is_directory(itr->status()))
        {
            if(itr->path().filename().extension().string() == fileExtension)
                listFiles.push_back(itr->path().string());
        }
    }
    return true;
}

void SoundEffectsManager::initializeInterfaceSounds()
{
    // Test wether the interface sounds are already loaded.
    if (mInterfaceSounds.empty() == false)
        return;

    Ogre::String soundFolderPath = ResourceManager::getSingletonPtr()->getSoundPath();

    // TODO: Dehard-code it and place the sound filename in a config file.
    mInterfaceSounds.push_back(new GameSound(soundFolderPath + "Click/click.ogg", false));   // BUTTONCLICK
    mInterfaceSounds.push_back(new GameSound(soundFolderPath + "Click/click.ogg", false));   // DIGSELECT
    mInterfaceSounds.push_back(new GameSound(soundFolderPath + "RoomBuild/bump.ogg", false)); // BUILDROOM
    mInterfaceSounds.push_back(new GameSound(soundFolderPath + "RoomBuild/bump.ogg", false)); // BUILDTRAP

    if (mRocksFallSounds.empty() == false)
        return;

    // Rock fall sounds
    std::vector<std::string> soundFilenames;
    fillFilesList(soundFolderPath + "RocksFalling/", soundFilenames, ".ogg");
    for (unsigned int i = 0; i < soundFilenames.size(); ++i)
    {
        mRocksFallSounds.push_back(new GameSound(soundFilenames[i], true));
    }

    if (mClaimedSounds.empty() == false)
        return;

    // Claim sounds
    soundFilenames.clear();
    fillFilesList(soundFolderPath + "ClaimTile/", soundFilenames, ".ogg");
    for (unsigned int i = 0; i < soundFilenames.size(); ++i)
    {
        mClaimedSounds.push_back(new GameSound(soundFilenames[i], true));
    }
}

void SoundEffectsManager::initializeDefaultCreatureSounds()
{
    if (mGameSoundCache.empty() == false)
        return;

    Ogre::String soundFolderPath = ResourceManager::getSingletonPtr()->getSoundPath();

    // Create and populate a new "Default" creature sound library
    CreatureSound* crSound = new CreatureSound();
    // TODO: Everything is hard-coded here, later we might want to make this more dynamic.

    // Attack sounds
    std::vector<std::string> soundFilenames;
    fillFilesList(soundFolderPath + "Sword/", soundFilenames, ".ogg");
    std::vector<GameSound*>& attackSounds = crSound->mSoundsPerType[CreatureSound::ATTACK];
    for (unsigned int i = 0; i < soundFilenames.size(); ++i)
    {
        GameSound* gm = new GameSound(soundFilenames[i], true);

        // Register it to the cache
        mGameSoundCache.insert(std::make_pair(gm->getFilename(), gm));

        // Add it to the attack sounds
        attackSounds.push_back(gm);
    }

    // Digging sounds
    soundFilenames.clear();
    fillFilesList(soundFolderPath + "Digging/", soundFilenames, ".ogg");
    std::vector<GameSound*>& diggingSounds = crSound->mSoundsPerType[CreatureSound::DIGGING];
    for (unsigned int i = 0; i < soundFilenames.size(); ++i)
    {
        GameSound* gm = new GameSound(soundFilenames[i], true);
        // Register it to the cache
        mGameSoundCache.insert(std::make_pair(gm->getFilename(), gm));
        // Add it to the digging sounds
        diggingSounds.push_back(gm);
    }

    // Pickup sounds - PICKUP
    // 1 sound atm...
    {
        std::vector<GameSound*>& pickupSounds = crSound->mSoundsPerType[CreatureSound::PICKUP];
        GameSound* gm = new GameSound(soundFolderPath + "Click/click.ogg", true);
        mGameSoundCache.insert(std::make_pair(gm->getFilename(), gm));
        pickupSounds.push_back(gm);
    }

    // Drop sounds - DROP
    // 1 sound atm...
    {
        std::vector<GameSound*>& dropSounds = crSound->mSoundsPerType[CreatureSound::DROP];
        GameSound* gm = new GameSound(soundFolderPath + "RoomBuild/bump.ogg", true);
        mGameSoundCache.insert(std::make_pair(gm->getFilename(), gm));
        dropSounds.push_back(gm);
    }

    // Idle sounds, none atm... - IDLE

    // Now that the default creature sounds have been created, we register
    // the new creature sound library in the corresponding cache.
    mCreatureSoundCache["Default"] = crSound;
}

void SoundEffectsManager::setListenerPosition(const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
{
    sf::Listener::setPosition(static_cast<float> (position.x),
                              static_cast<float> (position.y),
                              static_cast<float> (position.z));

    Ogre::Vector3 vDir = orientation.zAxis();
    sf::Listener::setDirection(-vDir.x, -vDir.y, -vDir.z);
}

void SoundEffectsManager::playInterfaceSound(InterfaceSound soundType)
{
    mInterfaceSounds[soundType]->play();
}

void SoundEffectsManager::playInterfaceSound(InterfaceSound soundType, const Ogre::Vector3& position)
{
    mInterfaceSounds[soundType]->setPosition(static_cast<float> (position.x),
                                             static_cast<float> (position.y),
                                             static_cast<float> (position.z));
    playInterfaceSound(soundType);
}

void SoundEffectsManager::playInterfaceSound(InterfaceSound soundType, int tileX, int tileY)
{
    mInterfaceSounds[soundType]->setPosition(static_cast<float> (tileX),
                                             static_cast<float> (tileY),
                                             TILE_ZPOS);
    playInterfaceSound(soundType);
}

void SoundEffectsManager::playRockFallingSound(int tileX, int tileY)
{
    //std::cout << "Play rock sound at: " << tileX << ", " << tileY << std::endl;
    unsigned int soundId = Random::Uint(0, mRocksFallSounds.size() - 1);
    if (soundId < 0)
        return;
    mRocksFallSounds[soundId]->setPosition(static_cast<float> (tileX),
                                           static_cast<float> (tileY),
                                           TILE_ZPOS);
    mRocksFallSounds[soundId]->play();
}

void SoundEffectsManager::playClaimedSound(int tileX, int tileY)
{
    //std::cout << "Play claim sound at: " << tileX << ", " << tileY << std::endl;
    unsigned int soundId = Random::Uint(0, mClaimedSounds.size() - 1);
    if (soundId < 0)
        return;
    mClaimedSounds[soundId]->setPosition(static_cast<float> (tileX),
                                        static_cast<float> (tileY),
                                        TILE_ZPOS);
    mClaimedSounds[soundId]->play();
}

CreatureSound* SoundEffectsManager::getCreatureClassSounds(const std::string& /*className*/)
{
    // For now, every creature shares the same creature sound library
    // TODO: Later, one will have to register the sounds according to the class name if not found here,
    // while making to check whether a sound isn't already in the game sound cache,
    // and then reuse in that case.
    return mCreatureSoundCache["Default"];
}

void SoundEffectsManager::createCreatureClassSounds(const std::string& /*className*/)
{
    // Not implemented yet
    // TODO: Later, one will have to register the sounds according to the class name,
    // while making to check whether a sound isn't already in the game sound cache,
    // and then reuse in that case.
}
