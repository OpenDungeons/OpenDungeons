/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
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

#include "sound/SoundEffectsManager.h"

#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/ResourceManager.h"
#include "utils/Random.h"
#include "utils/LogManager.h"
#include "network/ODPacket.h"

#include <OgreQuaternion.h>

// class GameSound
GameSound::GameSound(const std::string& filename, bool spatialSound):
    mSound(nullptr),
    mSoundBuffer(nullptr)
{
    // Loads the buffer
    mSoundBuffer = new sf::SoundBuffer();
    if (mSoundBuffer->loadFromFile(filename) == false)
    {
        delete mSoundBuffer;
        mSoundBuffer = nullptr;
        return;
    }

    mFilename = filename;

    mSound = new sf::Sound();
    // Loads the main sound object
    mSound->setBuffer(*mSoundBuffer);

    mSound->setLoop(false);

    // Sets a correct attenuation value if the sound is spatial
    if (spatialSound == true)
    {
        // Set convenient spatial fading unit.
        mSound->setVolume(100.0f);
        mSound->setAttenuation(3.0f);
        mSound->setMinDistance(3.0f);
    }
    else // Disable attenuation for sounds that must heard the same way everywhere
    {
        // Prevents the sound from being too loud
        mSound->setRelativeToListener(true);
        mSound->setVolume(30.0f);
        mSound->setAttenuation(0.0f);
    }
}

GameSound::~GameSound()
{
    // The sound object must be stopped and destroyed before its corresponding
    // buffer to ensure detaching the sound stream from the sound source.
    // This prevents a lot of warnings at app quit.
    if (mSound != nullptr)
    {
        mSound->stop();
        delete mSound;
    }

    if (mSoundBuffer != nullptr)
        delete mSoundBuffer;
}

void GameSound::play(float x, float y, float z)
{
    // Check whether the sound is spatial
    if (mSound->getAttenuation() > 0.0f)
    {
        // Check the distance against the listener height and cull the sound accordingly.
        // This permits to hear only the sound of the area seen in game,
        // and avoid glitches in heard sounds.
        sf::Vector3f lis = sf::Listener::getPosition();
        float distance2 = (lis.x - x) * (lis.x - x) + (lis.y - y) * (lis.y - y);
        double height2 = (lis.z * lis.z) + (lis.z * lis.z);

        if (distance2 > height2)
            return;
    }

    mSound->setPosition(x, y, z);
    mSound->play();
}

// SoundEffectsManager class
template<> SoundEffectsManager* Ogre::Singleton<SoundEffectsManager>::msSingleton = nullptr;

SoundEffectsManager::SoundEffectsManager()
{
    const std::string& soundFolderPath = ResourceManager::getSingleton().getSoundPath();
    // We read the spatial sound directory
    readSounds(mSpatialSounds, soundFolderPath + "Spatial/", "", true);

    // We read the relative sound directory
    for(const std::string& keeper : ConfigManager::getSingleton().getKeeperVoices())
    {
        readSounds(mRelativeSounds, soundFolderPath + "relative/" + keeper, keeper, false);
    }
}

SoundEffectsManager::~SoundEffectsManager()
{
    // Clear up every cached sounds...
    std::map<std::string, GameSound*>::iterator it = mGameSoundCache.begin();
    std::map<std::string, GameSound*>::iterator it_end = mGameSoundCache.end();
    for (; it != it_end; ++it)
    {
        if (it->second != nullptr)
            delete it->second;
    }
}

void SoundEffectsManager::readSounds(std::map<std::string, std::vector<GameSound*>>& soundsFamily,
        const std::string& parentPath, const std::string& parentFamily, bool spatialSound)
{
    std::vector<std::string> directories;
    if(!Helper::fillDirList(parentPath, directories, false))
    {
        OD_LOG_INF("Could not find sounds in directory=" + parentPath);
        return;
    }

    for(const std::string& directory : directories)
    {
        // We read sub directories
        std::string fullDir = parentPath + "/" + directory;
        std::string fullFamily = parentFamily.empty() ? directory : parentFamily + "/" + directory;
        readSounds(soundsFamily, fullDir, fullFamily, spatialSound);

        std::vector<std::string> soundFilenames;
        if(!Helper::fillFilesList(fullDir, soundFilenames, ".ogg"))
        {
            OD_LOG_ERR("Cannot load sounds from=" + fullDir);
            continue;
        }

        // We do not create entries for empty directories
        if(soundFilenames.empty())
            continue;

        std::vector<GameSound*>& sounds = soundsFamily[fullFamily];
        for(const std::string& soundFilename : soundFilenames)
        {
            GameSound* gm = getGameSound(soundFilename, spatialSound);
            if (gm == nullptr)
            {
                OD_LOG_ERR("Cannot load sound=" + soundFilename);
                continue;
            }

            OD_LOG_INF("Sound loaded family=" + fullFamily + ", filename=" + soundFilename);
            sounds.push_back(gm);
        }
    }
}

void SoundEffectsManager::updateListener(float timeSinceLastFrame,
        const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
{
    sf::Listener::setPosition(static_cast<float> (position.x),
                              static_cast<float> (position.y),
                              static_cast<float> (position.z));

    Ogre::Vector3 vDir = orientation.zAxis();
    sf::Listener::setDirection(-vDir.x, -vDir.y, -vDir.z);

    // We launch the next pending relative sound if any
    if(mRelativeSoundQueue.empty())
        return;

    auto it = mRelativeSoundQueue.begin();
    GameSound* sound = *it;
    if(sound->isPlaying())
        return;

    mRelativeSoundQueue.erase(it);
    if(mRelativeSoundQueue.empty())
        return;

    mRelativeSoundQueue[0]->play();
}

void SoundEffectsManager::playSpatialSound(const std::string& family,
        float XPos, float YPos, float height)
{
    auto it = mSpatialSounds.find(family);
    if(it == mSpatialSounds.end())
    {
        OD_LOG_ERR("Couldn't find sound family=" + family);
        return;
    }

    std::vector<GameSound*>& sounds = it->second;
    if(sounds.empty())
    {
        OD_LOG_ERR("No sound found for sound family=" + family);
        return;
    }

    unsigned int soundId = Random::Uint(0, sounds.size() - 1);
    sounds[soundId]->play(XPos, YPos, height);
}

void SoundEffectsManager::playRelativeSound(const std::string& family)
{
    // We search for the selected sound in the currently selected voice group. If we cannot
    // find it, we fall down to the default group. That allows to create new voice groups that
    // do not have to have sound for every event
    std::string keeperVoice = ConfigManager::getSingleton().getGameValue(Config::KEEPERVOICE);
    auto it = mRelativeSounds.find(keeperVoice + "/" + family);
    if(it == mRelativeSounds.end())
    {
        it = mRelativeSounds.find(ConfigManager::DEFAULT_KEEPER_VOICE + "/" + family);
        if(it == mRelativeSounds.end())
        {
            OD_LOG_ERR("Couldn't find sound family=" + family);
            return;
        }
    }

    std::vector<GameSound*>& sounds = it->second;
    if(sounds.empty())
        return;

    unsigned int soundId = Random::Uint(0, sounds.size() - 1);
    GameSound* sound = sounds[soundId];
    if(mRelativeSoundQueue.empty())
        sound->play();

    mRelativeSoundQueue.push_back(sound);
}

GameSound* SoundEffectsManager::getGameSound(const std::string& filename, bool spatialSound)
{
    // We add a suffix to the sound filename as two versions of it can be kept, one spatial,
    // and one which isn't.
    std::string soundFile = filename + (spatialSound ? "_spatial": "");

    std::map<std::string, GameSound*>::iterator it = mGameSoundCache.find(soundFile);
    // Create a new game sound instance when the sound doesn't exist and register it.
    if (it == mGameSoundCache.end())
    {
        GameSound* gm = new GameSound(filename, spatialSound);
        if (gm->isInitialized() == false)
        {
            // Invalid sound filename
            delete gm;
            return nullptr;
        }

        mGameSoundCache.insert(std::make_pair(soundFile, gm));
        return gm;
    }

    return it->second;
}
