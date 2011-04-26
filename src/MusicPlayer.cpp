/*!
* \file   MusicPlayer.cpp
* \author oln, StefanP.MUC
* \date   November 10 2010
* \brief  Class "MusicPlayer" containing everything to play music tracks.
*/

#include <iostream>

#include <OgreResourceGroupManager.h>

#include "Functions.h"
#include "ResourceManager.h"
#include "Random.h"

#include "MusicPlayer.h"

template<> MusicPlayer* Ogre::Singleton<MusicPlayer>::ms_Singleton = 0;

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

    std::cout << "Loading music..." << std::endl;

    //Get list of files in the resource.
    Ogre::StringVectorPtr musicFiles =
            Ogre::ResourceGroupManager::getSingleton().listResourceNames("Music");
    tracks.reserve(musicFiles->size());

    std::string path = ResourceManager::getSingletonPtr()->getMusicPath();
    /* Create sound objects for all files, Sound objects should be deleted
     * automatically by the sound manager.
     */
    for(Ogre::StringVector::iterator it = musicFiles->begin(), end = musicFiles->end();
         it != end; ++it)
    {
        std::cout << path << "/" << *it << std::endl;
        Ogre::SharedPtr<sf::Music> track(new sf::Music());
        //TODO - check for text encoding issues.
        if(track->OpenFromFile(path + "/" + *it))
        {
            track->SetVolume(25.0f);
            track->SetAttenuation(0);
            tracks.push_back(track);
        }
    }

    if(tracks.size() == 0)
    {
        std::cerr << "No music files loaded... no music will be played"
            << std::endl;
    }
    else
    {
        std::cout << "Music loading done" << std::endl;
        loaded = true;
    }
}

MusicPlayer::~MusicPlayer()
{
}

MusicPlayer& MusicPlayer::getSingleton()
{
    return *ms_Singleton;
}

MusicPlayer* MusicPlayer::getSingletonPtr()
{
    assert(ms_Singleton);
    return ms_Singleton;
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
    if(loaded && (tracks[currentTrack]->GetStatus() == sf::Sound::Stopped))
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
        tracks[currentTrack]->Stop();
        currentTrack = trackNumber;
        tracks[currentTrack]->Play();
    }
}

/** \brief Skip to the next track
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
        	newTrack = randomUint(0, tracks.size() - 1);
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
