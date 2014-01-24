/*!
* \file   MusicPlayer.cpp
* \author oln, StefanP.MUC
* \date   November 10 2010
* \brief  Class "MusicPlayer" containing everything to play music tracks.
*/

#include <iostream>

#include <OgreResourceGroupManager.h>

#include "LogManager.h"
#include "ResourceManager.h"
#include "Random.h"

#include "MusicPlayer.h"

template<> MusicPlayer* Ogre::Singleton<MusicPlayer>::msSingleton = 0;

/*! \brief Initialize variables and load music files in the resource
 *  locations listed under "Music".
 */
MusicPlayer::MusicPlayer() :
            loaded(false),
            randomized(false),
            currentTrack(0)
{
    /* TODO: this should be changed somehow. Storing only tracknumbers
     *       doesn't allow us to play a specific music in a special situation,
     *       because we won't know which number it has.
     */

    LogManager& logMgr = LogManager::getSingleton();
    logMgr.logMessage("Loading music...");

    //Get list of files in the resource.
    Ogre::StringVectorPtr musicFiles = ResourceManager::getSingleton().
            listAllMusicFiles();
    tracks.reserve(musicFiles->size());

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
            tracks.push_back(track);
        }
    }

    if(tracks.empty())
    {
        logMgr.logMessage("No music files loaded... no music will be played",
                Ogre::LML_CRITICAL);
    }
    else
    {
        logMgr.logMessage("Music loading done");
        loaded = true;
    }
}

/** \brief Check if current track is finished, and change to next track if it is.
 *
 */
void MusicPlayer::update()
{
    /* TODO: after upgrading to SFML 2.0, we can use sf::Music::OnGetData()
     * to achieve this instead of calling update() on every frame
     * (in 1.6 it's private, but in 2.0 it's protected, so we then can
     * override it)
     */
    if(loaded && (tracks[currentTrack]->getStatus() == sf::Sound::Stopped))
    {
        next();
    }
}

/** \brief Start music playback with trackNumber if any music is loaded.
 *  \param  trackNumber number of the Track to play
 */
void MusicPlayer::start(const unsigned int& trackNumber)
{
    if(loaded)
    {
        tracks[currentTrack]->stop();
        currentTrack = trackNumber;
        tracks[currentTrack]->play();
    }
}

/** \brief Skip to the next track or a new random track
 *
 */
void MusicPlayer::next()
{
    unsigned int newTrack = currentTrack;

    if(randomized)
    {
        newTrack = Random::Uint(0, tracks.size() - 1);
        /* TODO: After we have more than one track make sure that the same
         * track isn't loaded twice
         *
         * Code for this:
         * TODO: Don't forget to remove the line above then
        while(newTrack == currentTrack)
        {
        	newTrack = Random::Uint(0, tracks.size() - 1);
        }
         */
    }
    else
    {
        if(++newTrack >= tracks.size())
        {
            newTrack = 0;
        }
    }

    start(newTrack);
}

MusicPlayer::~MusicPlayer()
{
}
