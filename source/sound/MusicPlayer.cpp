/*!
 * \file   MusicPlayer.cpp
 * \author oln, StefanP.MUC, Bertram
 * \date   November 10 2010
 * \brief  Class "MusicPlayer" containing everything to play music tracks.
 *
 *  Copyright (C) 2010-2015  OpenDungeons Team
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

#include "sound/MusicPlayer.h"

#include "utils/LogManager.h"
#include "utils/ResourceManager.h"

#include <OgreResourceGroupManager.h>

//! \brief The max music volume value.
const float MAX_VOLUME = 25.0f;

// ODMusic class
ODMusic::ODMusic(const std::string& filename)
{
    loadFromFile(filename);
}

bool ODMusic::loadFromFile(const std::string& filename)
{
    if (mMusic.openFromFile(filename) == false)
    {
        mMusicState = UNLOADED;
        return false;
    }

    // Set convenient values for our use.
    mMusic.setVolume(MAX_VOLUME);
    mMusic.setAttenuation(0);
    mMusic.setLoop(true);

    mMusicState = STOPPED;
    return true;
}

void ODMusic::play()
{
    if (mMusicState == UNLOADED)
        return;
    if (mMusicState != STOPPED && mMusicState != FADE_OUT)
        return;

    mMusic.setVolume(MAX_VOLUME);
    mMusic.play();
    mMusicState = PLAYING;
}

void ODMusic::stop()
{
    if (mMusicState == UNLOADED)
        return;
    if (mMusicState != PLAYING && mMusicState != FADE_IN)
        return;

    mMusic.setVolume(0.0f);
    mMusic.stop();
    mMusicState = STOPPED;
}

void ODMusic::fadeIn()
{
    if (mMusicState == UNLOADED)
        return;

    if (mMusicState != STOPPED && mMusicState != FADE_OUT)
        return;

    mMusic.play();
    mMusicState = FADE_IN;
}

void ODMusic::fadeOut()
{
    if (mMusicState == UNLOADED)
        return;
    if (mMusicState != PLAYING && mMusicState != FADE_IN)
        return;

    if (mMusic.getVolume() > 0.0f)
    {
        mMusicState = FADE_OUT;
    }
    else
    {
        mMusic.stop();
        mMusicState = STOPPED;
    }
}

void ODMusic::update(float timeSinceLastUpdate)
{
    if (mMusicState == UNLOADED || mMusicState == STOPPED || mMusicState == PLAYING)
        return;

    // Handles fade in/outs
    if (mMusicState == FADE_IN)
    {
        // Make the music volume gradually fade in
        float volume = mMusic.getVolume();

        volume += timeSinceLastUpdate * 10.0f;
        if (volume >= MAX_VOLUME)
        {
            mMusic.setVolume(MAX_VOLUME);
            mMusicState = PLAYING;
            return;
        }
        else
        {
            mMusic.setVolume(volume);
        }
    }
    else if (mMusicState == FADE_OUT)
    {
        // Make the music volume gradually fade in
        float volume = mMusic.getVolume();

        volume -= timeSinceLastUpdate * 10.0f;
        if (volume <= 0.0f)
        {
            mMusic.setVolume(0.0f);
            mMusic.stop();
            mMusicState = STOPPED;
            return;
        }
        else
        {
            mMusic.setVolume(volume);
        }
    }
}


// MusicPlayer class
template<> MusicPlayer* Ogre::Singleton<MusicPlayer>::msSingleton = 0;

MusicPlayer::MusicPlayer() :
    mLoaded(false),
    mCurrentTrack(std::string()),
    mIsPlaying(false)
{
    LogManager& logMgr = LogManager::getSingleton();
    logMgr.logMessage("Loading music...");

    //Get list of files in the resource.
    Ogre::StringVectorPtr musicFiles = ResourceManager::getSingleton().listAllMusicFiles();

    std::string path = ResourceManager::getSingletonPtr()->getMusicPath();
    /* Create sound objects for all files, Sound objects should be deleted
     * automatically by the sound manager.
     */
    for(Ogre::StringVector::iterator it = musicFiles->begin(),
            end = musicFiles->end(); it != end; ++it)
    {
        ODMusic* track = new ODMusic();
        std::string trackName = *it;
        logMgr.logMessage("Loading: " + trackName);

        if(track->loadFromFile(path + trackName))
        {
            mTracks.insert(std::make_pair(trackName, track));
        }
        else
        {
            // Prevents invalid tracks from leaking memory
            delete track;
        }
    }

    if(mTracks.empty())
    {
        logMgr.logMessage("No music files loaded... no music will be played",
                Ogre::LML_NORMAL);
    }
    else
    {
        logMgr.logMessage("Music loading done");
        mLoaded = true;
    }
}

MusicPlayer::~MusicPlayer()
{
    // Delete the OD Music.
    std::map<std::string, ODMusic*>::iterator it = mTracks.begin();
    std::map<std::string, ODMusic*>::iterator it_end = mTracks.end();
    for (; it != it_end; ++it)
        delete it->second;
}

void MusicPlayer::update(float timeSinceLastUpdate)
{
    if(!mLoaded)
        return;

    // Handle fade ins/fade outs.
    std::map<std::string, ODMusic*>::iterator it = mTracks.begin();
    std::map<std::string, ODMusic*>::iterator it_end = mTracks.end();
    for (; it != it_end; ++it)
        it->second->update(timeSinceLastUpdate);
}

void MusicPlayer::play(const std::string& trackName)
{
    if(!mLoaded)
        return;

    // If the music is already playing, then don't restart it.
    if (mIsPlaying && mCurrentTrack == trackName)
        return;

    if (mIsPlaying)
    {
        mTracks[mCurrentTrack]->fadeOut();
        mIsPlaying = false;
    }

    if (mTracks.find(trackName) == mTracks.end())
        return;

    mCurrentTrack = trackName;
    mTracks[mCurrentTrack]->fadeIn();
    mIsPlaying = true;
}

void MusicPlayer::stop()
{
    if(!mLoaded || !mIsPlaying)
        return;

    mTracks[mCurrentTrack]->fadeOut();
    mIsPlaying = false;
}
