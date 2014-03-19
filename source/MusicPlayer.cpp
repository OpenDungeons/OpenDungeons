/*!
 * \file   MusicPlayer.cpp
 * \author oln, StefanP.MUC
 * \date   November 10 2010
 * \brief  Class "MusicPlayer" containing everything to play music tracks.
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

#include "MusicPlayer.h"

#include "LogManager.h"
#include "ResourceManager.h"
#include "Random.h"

#include <OgreResourceGroupManager.h>

template<> MusicPlayer* Ogre::Singleton<MusicPlayer>::msSingleton = 0;

MusicPlayer::MusicPlayer() :
    mLoaded(false),
    mRandomized(false),
    mCurrentTrack(0),
    mIsPlaying(false)
{
    /* TODO: this should be changed somehow. Storing only tracknumbers
     *       doesn't allow us to play a specific music in a special situation,
     *       because we won't know which number it has.
     */

    LogManager& logMgr = LogManager::getSingleton();
    logMgr.logMessage("Loading music...");

    //Get list of files in the resource.
    Ogre::StringVectorPtr musicFiles = ResourceManager::getSingleton().listAllMusicFiles();
    mTracks.reserve(musicFiles->size());

    std::string path = ResourceManager::getSingletonPtr()->getMusicPath();
    /* Create sound objects for all files, Sound objects should be deleted
     * automatically by the sound manager.
     */
    for(Ogre::StringVector::iterator it = musicFiles->begin(),
            end = musicFiles->end(); it != end; ++it)
    {
        logMgr.logMessage(path + "/" + *it);
        Ogre::SharedPtr<sf::Music> track(new sf::Music());
        //TODO - check for text encoding issues.
        if(track->openFromFile(path + "/" + *it))
        {
            track->setVolume(25.0f);
            track->setAttenuation(0);
            mTracks.push_back(track);
        }
    }

    if(mTracks.empty())
    {
        logMgr.logMessage("No music files loaded... no music will be played",
                Ogre::LML_CRITICAL);
    }
    else
    {
        logMgr.logMessage("Music loading done");
        mLoaded = true;
    }
}

MusicPlayer::~MusicPlayer()
{
}

void MusicPlayer::update()
{
    if(!mLoaded)
        return;

    // We don't play the next music if we decided not to play any music.
    if (!mIsPlaying)
        return;

    /* TODO: after upgrading to SFML 2.0, we can use sf::Music::OnGetData()
     * to achieve this instead of calling update() on every frame
     * (in 1.6 it's private, but in 2.0 it's protected, so we then can
     * override it)
     */
    if(mTracks[mCurrentTrack]->getStatus() == sf::Music::Stopped)
    {
        mIsPlaying = false;
        next();
    }
}

void MusicPlayer::start(const unsigned int& trackNumber)
{
    if(!mLoaded)
        return;

    // If the music is already playing, then don't restart it.
    if (mIsPlaying && mCurrentTrack == trackNumber)
        return;

    if (mIsPlaying)
        mTracks[mCurrentTrack]->stop();
    mCurrentTrack = trackNumber;
    mTracks[mCurrentTrack]->play();
    mIsPlaying = true;
}

void MusicPlayer::stop()
{
    if(!mLoaded || !mIsPlaying)
        return;

    mTracks[mCurrentTrack]->stop();
    mIsPlaying = false;
}

void MusicPlayer::next()
{
    unsigned int newTrack = mCurrentTrack;

    // if there is only one track, we just keep on playing it
    unsigned int numTracks = mTracks.size();
    if (numTracks == 1)
    {
        start(0);
        return;
    }

    if(mRandomized)
    {
        while(newTrack == mCurrentTrack)
        {
            newTrack = Random::Uint(0, numTracks - 1);
        }
    }
    else
    {
        if(++newTrack >= mTracks.size())
        {
            newTrack = 0;
        }
    }

    start(newTrack);
}
