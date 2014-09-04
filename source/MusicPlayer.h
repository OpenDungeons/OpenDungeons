/*!
 * \file   MusicPlayer.h
 * \author oln, StefanP.MUC, Bertram
 * \date   November 10 2010
 * \brief  Header of class "MusicPlayer" containing everything to play
 *         music tracks.
 *
 *  Copyright (C) 2010-2014  OpenDungeons Team
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

#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <SFML/Audio.hpp>
#include <OgreSingleton.h>

#include <map>
#include <string>

//! \brief A small class adding fade in/out support over sf::Music
class ODMusic
{
public:
    ODMusic(const std::string& filename);

    ODMusic():
        mMusicState(UNLOADED)
    {}

    ~ODMusic()
    { mMusic.stop(); }

    enum MusicState
    {
        UNLOADED = 0,
        STOPPED,
        PLAYING,
        FADE_IN,
        FADE_OUT
    };

    bool loadFromFile(const std::string& filename);

    void play();
    void stop();

    void fadeIn();
    void fadeOut();

    //! \brief Handles potential fade in/out updates
    //! \param timeSinceLastUpdate time in seconds since the last update.
    //! \note The parameter comes from Ogre frameListener. See Ogre::FrameEvent
    //! This is used to make the fade in/out framerate independent.
    void update(float timeSinceLastUpdate);

private:
    //! \brief The SFML music object
    sf::Music mMusic;

    //! \brief The music current state
    MusicState mMusicState;
};

//! \brief Class to manage playing of music.
class MusicPlayer: public Ogre::Singleton<MusicPlayer>
{
public:
    /*! \brief Initialize variables and load music files in the resource
     *  locations listed under "Music".
     */
    MusicPlayer();

    virtual ~MusicPlayer();

    //! \brief Takes care of updating the music states in case of fade in/out.
    //! \param timeSinceLastUpdate time in seconds since the last update.
    //! \note The parameter comes from Ogre frameListener. See Ogre::FrameEvent
    //! This is used to make the fade in/out framerate independent.
    void update(float timeSinceLastUpdate);

    /** \brief Start music playback with a fade in effect.
     *  \param trackNname name of the track to play
     *  \note The previous track is faded out.
     */
    void play(const std::string& trackName);

    //! \brief Stops the currently played music with a fade out.
    void stop();

private:
    //! \brief The tracks names and corresponding list
    std::map<std::string, ODMusic*> mTracks;

    //! \brief Tells whether the tracks are loaded in memory.
    bool mLoaded;

    //! \brief The currently played track index
    std::string mCurrentTrack;

    //! \brief Whether the track is actually playing
    bool mIsPlaying;
};

#endif // MUSICPLAYER_H
