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

#ifndef SOUNDEFFECTSMANAGER_H_
#define SOUNDEFFECTSMANAGER_H_

#include <OgreSingleton.h>
#include <OgreVector3.h>
#include <SFML/Audio.hpp>
#include <vector>
#include <map>

// Forward declarations
class CreatureDefinition;
class ODPacket;
namespace Ogre
{
    class Quaternion;
}

namespace InterfaceSounds
{
    const std::string Click = "Click";
    const std::string PickSelector = "PickSelector";
}

namespace GameSounds
{
    const std::string DepositGold = "DepositGold";
}

// The Z value to use for tile positioned sounds.
// Tiles are from -0.25 to 3.0 in z value, the floor is at 0,
// and we're using an pseudo-average value.
const float TILE_ZPOS = 2.5;

//! \brief A small object used to contain both the sound and its buffer,
//! as both  must have the same life-cycle.
class GameSound
{
public:
    //! \brief Game sound constructor
    //! \param filename The sound filename used to load the sound.
    //! \param spatialSound tells whether the sound should be prepared to be a spatial sound.
    //! with a position, strength and attenuation relative to the camera position.
    GameSound(const std::string& filename, bool spatialSound);

    ~GameSound();

    bool isInitialized() const
    { return !(mSoundBuffer == nullptr); }

    //! \brief Play at the given spatial position
    void play(float x, float y, float z);

    void stop()
    { mSound->stop(); }

    const std::string& getFilename() const
    { return mFilename; }

private:
    //! \brief The Main sound object
    sf::Sound* mSound;

    //! \brief The corresponding sound buffer, must not be destroyed
    //! before the sound object itself is deleted.
    sf::SoundBuffer* mSoundBuffer;

    //! \brief The sound filename
    std::string mFilename;
};

//! \brief The different interface sound types.
enum class SpatialSoundType
{
    Interface = 0,
    Game,
    Creatures,
    Rooms,
    Traps,
    Spells,
    nbSounds
};

//! \brief Used to transfer SpatialSounds type over the network.
ODPacket& operator<<(ODPacket& os, const SpatialSoundType& st);
ODPacket& operator>>(ODPacket& is, SpatialSoundType& st);

//! \brief Helper class to manage sound effects.
class SoundEffectsManager: public Ogre::Singleton<SoundEffectsManager>
{
public:
    //! \brief Loads every available interface sounds
    SoundEffectsManager();

    //! \brief Deletes both sound caches.
    virtual ~SoundEffectsManager();

    //! \brief Init the interface sounds.
    void initializeSpatialSounds();

    void setListenerPosition(const Ogre::Vector3& position, const Ogre::Quaternion& orientation);

    //! \brief Plays a spatial sound at the given tile position.
    void playSpatialSound(SpatialSoundType soundType, const std::string& family,
        float XPos, float YPos, float height = TILE_ZPOS);

    //! \brief Proxy used for sounds that aren't spatial and can be heard everywhere.
    void playSpatialSound(SpatialSoundType soundType, const std::string& family)
    { playSpatialSound(soundType, family, 0.0f, 0.0f); }

    void playSpatialSound(SpatialSoundType soundType, const std::string& family, const Ogre::Vector3& position)
    { playSpatialSound(soundType, family, static_cast<float>(position.x), static_cast<float>(position.y), static_cast<float>(position.z)); }

private:
    //! \brief Every game sounds (interface or creature). The sounds are read when launching the
    //! game by browsing the sound directory
    //! \note the GameSound here are handled by the game sound cache.
    std::vector<std::map<std::string, std::vector<GameSound*>>> mSpatialSounds;

    //! \brief The sound cache, containing the sound references, used by game entities.
    //! \brief The GameSounds here must be deleted at destruction.
    std::map<std::string, GameSound*> mGameSoundCache;

    //! \brief Returns a game sounds from the cache.
    //! \param filename The sound filename.
    //! \param spatialSound Whether the sound is a spatial sound.
    //! If an unexisting file is given, a new cache instance is returned.
    //! \note Use this function only to create new game sounds as it is the only way to make sure
    //! the GameSound* instance is correclty cleared up when quitting.
    //! \warning Returns nullptr if the filename is an invalid/unreadable sound.
    GameSound* getGameSound(const std::string& filename, bool spatialSound = false);

    //! \brief Reads the path associated to the given soundType and fills the corresponding map
    void readSounds(SpatialSoundType soundType);

    //! \brief Recursive function that fills the sound map with sound files found and reads
    //! child directories
    void readSounds(std::map<std::string, std::vector<GameSound*>>& soundsFamily,
        const std::string& parentPath, const std::string& parentFamily, bool spatialSound);
};

#endif // SOUNDEFFECTSMANAGER_H_
