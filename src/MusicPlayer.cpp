#include "MusicPlayer.h"

template<> MusicPlayer* Ogre::Singleton<MusicPlayer>::ms_Singleton = 0;

/** \brief Initialise variables.
 *
 */
MusicPlayer::MusicPlayer() :
    loaded(false), currentTrack(0)
{

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
     * to achieve this instead of calling upgrade() on every frame
     * (in 1.6 it's private, but in 2.O it's protected, so we then can
     * override it)
     */
    if(loaded && (tracks[currentTrack]->GetStatus() == sf::Sound::Stopped))
    {
        next();
    }
}

/** \brief Initialise and load music files in the resource locations listed under "Music".
 *
 */
void MusicPlayer::load(const Ogre::String& path)
{
    if (!loaded)
    {
        std::cout << "Loading music..." << std::endl;

        //Get list of files in the resource.
        Ogre::StringVectorPtr musicFiles =
                Ogre::ResourceGroupManager::getSingleton().listResourceNames("Music");
        tracks.reserve(musicFiles->size());

        for(Ogre::StringVector::iterator it = musicFiles->begin(), end = musicFiles->end();
             it != end; ++it)
        {
            std::cout << path << "/" << *it << std::endl;
            //Create sound objects for all files, Sound objects should be deleted automatically
            //by the sound manager.
            //TODO - check what this does if something goes wrong loading the file.
            Ogre::SharedPtr<sf::Music> track(new sf::Music());
            //TODO - check for text encoding issues.
            if(track->OpenFromFile(path + "/" + *it))
            {
                track->SetVolume(25);
                track->SetAttenuation(0);
                tracks.push_back(track);
                //sound->disable3D(true); //Disable 3D sound for music files.
                //Stereo files are not positioned anyway, but in case we have mono music... this is necessary.

                //Lower volume to make it more in line with effects sounds.
                //sound->setVolume(0.25);
            }

        }

        if(tracks.size() == 0)
        {
            std::cerr << "No music files loaded... no music will be played"
                << std::endl;
        }
        else
        {
            //If there was any music loaded, store this.
            std::cout << "Loaded music" << std::endl;
            loaded = true;
        }
    }
}

/** \brief Start music playback with the currentTrack if any music is loaded.
 *
 */
void MusicPlayer::start()
{
    if(loaded)
    {
        tracks[currentTrack]->Play();
    }
}

/** \brief Skip to the next track
 *
 */
void MusicPlayer::next()
{
    if(++currentTrack >= tracks.size())
    {
        currentTrack = 0;
    }
    start();
}

//TODO: add random track selecting
