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

SoundEffectsManager::SoundEffectsManager() :
    mSpacialSounds(static_cast<uint32_t>(SpacialSound::NUM_INTERFACE_SOUNDS))
{
    initializeSpacialSounds();
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

void SoundEffectsManager::initializeSpacialSounds()
{
    // Test wether the interface sounds are already loaded.
    if (mSpacialSounds.empty() == false)
        return;

    Ogre::String soundFolderPath = ResourceManager::getSingletonPtr()->getSoundPath();

    // TODO: Dehard-code it and place the sound filename in a config file.
    // Only one click sounds atm...
    {
        GameSound* gm = getGameSound(soundFolderPath + "Game/click.ogg", false);
        if (gm != nullptr)
            mSpacialSounds[static_cast<uint32_t>(SpacialSound::BUTTONCLICK)].push_back(gm);
    }

    // Only one dig select sound atm...
    {
        GameSound* gm = getGameSound(soundFolderPath + "Game/PickSelector.ogg", false);
        if (gm != nullptr)
            mSpacialSounds[static_cast<uint32_t>(SpacialSound::DIGSELECT)].push_back(gm);
    }

    // Only one build room sound atm...
    {
        GameSound* gm = getGameSound(soundFolderPath + "Rooms/default_build_room.ogg", false);
        if (gm != nullptr)
            mSpacialSounds[static_cast<uint32_t>(SpacialSound::BUILDROOM)].push_back(gm);
    }

    // Only one build trap sound atm...
    {
        GameSound* gm = getGameSound(soundFolderPath + "Rooms/default_build_trap.ogg", false);
        if (gm != nullptr)
            mSpacialSounds[static_cast<uint32_t>(SpacialSound::BUILDTRAP)].push_back(gm);
    }

    // Cannon firing sound
    {
        GameSound* gm = getGameSound(soundFolderPath + "Traps/cannon_firing.ogg", false);
        if (gm != nullptr)
            mSpacialSounds[static_cast<uint32_t>(SpacialSound::CANNONFIRING)].push_back(gm);
    }

    // Rock falling sounds
    std::vector<std::string> soundFilenames;
    Helper::fillFilesList(soundFolderPath + "Game/RocksFalling/", soundFilenames, ".ogg");
    for (unsigned int i = 0; i < soundFilenames.size(); ++i)
    {
        GameSound* gm = getGameSound(soundFilenames[i], true);
        if (gm != nullptr)
            mSpacialSounds[static_cast<uint32_t>(SpacialSound::ROCKFALLING)].push_back(gm);
    }

    // Claim sounds
    soundFilenames.clear();
    Helper::fillFilesList(soundFolderPath + "Game/ClaimTile/", soundFilenames, ".ogg");
    for (unsigned int i = 0; i < soundFilenames.size(); ++i)
    {
        GameSound* gm = getGameSound(soundFilenames[i], true);
        if (gm != nullptr)
            mSpacialSounds[static_cast<uint32_t>(SpacialSound::CLAIMED)].push_back(gm);
    }

    // Deposit gold sounds
    soundFilenames.clear();
    Helper::fillFilesList(soundFolderPath + "Game/DepositGold/", soundFilenames, ".ogg");
    for (unsigned int i = 0; i < soundFilenames.size(); ++i)
    {
        GameSound* gm = getGameSound(soundFilenames[i], true);
        if (gm != nullptr)
            mSpacialSounds[static_cast<uint32_t>(SpacialSound::DEPOSITGOLD)].push_back(gm);
    }
}

void SoundEffectsManager::setListenerPosition(const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
{
    sf::Listener::setPosition(static_cast<float> (position.x),
                              static_cast<float> (position.y),
                              static_cast<float> (position.z));

    Ogre::Vector3 vDir = orientation.zAxis();
    sf::Listener::setDirection(-vDir.x, -vDir.y, -vDir.z);
}

void SoundEffectsManager::playSpacialSound(SpacialSound soundType, float XPos, float YPos, float height)
{
    uint32_t indexSound = static_cast<uint32_t>(soundType);
    if(indexSound >= mSpacialSounds.size())
    {
        OD_LOG_ERR("sound=" + Helper::toString(indexSound) + ", size=" + Helper::toString(mSpacialSounds.size()));
        return;
    }

    std::vector<GameSound*>& sounds = mSpacialSounds[indexSound];
    if(sounds.empty())
        return;

    unsigned int soundId = Random::Uint(0, sounds.size() - 1);
    sounds[soundId]->play(XPos, YPos, height);
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

ODPacket& operator<<(ODPacket& os, const SpacialSound& st)
{
    os << static_cast<int32_t>(st);
    return os;
}

ODPacket& operator>>(ODPacket& is, SpacialSound& st)
{
    int32_t tmp;
    is >> tmp;
    st = static_cast<SpacialSound>(tmp);
    return is;
}
