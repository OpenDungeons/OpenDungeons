/*!
 * \file   MusicPlayer.h
 * \author oln, StefanP.MUC
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
#include <OgreSharedPtr.h>
#include <OgreSingleton.h>

#include <vector>

//! \brief Class to manage playing of music.
class MusicPlayer: public Ogre::Singleton<MusicPlayer>
{
public:
    /*! \brief Initialize variables and load music files in the resource
     *  locations listed under "Music".
     */
    MusicPlayer();

    virtual ~MusicPlayer();

    //! \brief Check if current track is finished, and change to next track if it is.
    void update();

    /** \brief Start music playback with trackNumber if any music is loaded.
     *  \param  trackNumber number of the Track to play
     */
    void start(const unsigned int& trackNumber);

    //! \brief Stops the currently played music
    void stop();

    //! \brief Skip to the next track or a new random track
    void next();

    //! \brief Tells whether the tracks will be played in a random order.
    inline const bool& isRandomized() const
    { return mRandomized; }

    //! \brief Sets whether the tracks will be played in a random order.
    inline void setRandomize(const bool& randomize)
    { mRandomized = randomize; }

private:
    std::vector<Ogre::SharedPtr<sf::Music> > mTracks;
    bool mLoaded;
    bool mRandomized;

    //! \brief The currently played track index
    unsigned int mCurrentTrack;

    //! \brief Whether the track is actually playing
    bool mIsPlaying;
};

#endif // MUSICPLAYER_H
