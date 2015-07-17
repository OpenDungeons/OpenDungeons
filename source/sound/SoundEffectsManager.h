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
    GameSound(const std::string& filename, bool spatialSound = true);

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
enum class SpacialSound
{
    BUTTONCLICK = 0,
    DIGSELECT,
    BUILDROOM,
    BUILDTRAP,
    ROCKFALLING,
    CLAIMED,
    DEPOSITGOLD,
    CANNONFIRING,
    CREATURE_DROP,
    CREATURE_DIGGING,
    CREATURE_ATTACK,
    NUM_INTERFACE_SOUNDS
};

//! \brief Used to transfer SpacialSounds type over the network.
ODPacket& operator<<(ODPacket& os, const SpacialSound& st);
ODPacket& operator>>(ODPacket& is, SpacialSound& st);

//! \brief Helper class to manage sound effects.
class SoundEffectsManager: public Ogre::Singleton<SoundEffectsManager>
{
public:
    //! \brief Loads every available interface sounds
    SoundEffectsManager();

    //! \brief Deletes both sound caches.
    virtual ~SoundEffectsManager();

    //! \brief Init the interface sounds.
    void initializeSpacialSounds();

    void setListenerPosition(const Ogre::Vector3& position, const Ogre::Quaternion& orientation);

    //! \brief Plays a spatial sound at the given tile position.
    void playSpacialSound(SpacialSound soundType, float XPos, float YPos, float height = TILE_ZPOS);

    //! \brief Proxy used for sounds that aren't spatial and can be heard everywhere.
    void playSpacialSound(SpacialSound soundType)
    { playSpacialSound(soundType, 0.0f, 0.0f); }

    void playSpacialSound(SpacialSound soundType, const Ogre::Vector3& position)
    { playSpacialSound(soundType, static_cast<float>(position.x), static_cast<float>(position.y), static_cast<float>(position.z)); }

private:
    //! \brief Every interface or generic in game sounds
    //! \note the GameSound here are handled by the game sound cache.
    std::vector<std::vector<GameSound*> > mSpacialSounds;

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

    //! \brief The list of available sound to play per type.
    std::vector<std::pair<std::vector<GameSound*>,int>> mSoundsPerType;
};

#endif // SOUNDEFFECTSMANAGER_H_
