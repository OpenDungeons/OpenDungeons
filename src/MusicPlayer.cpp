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
    if(loaded)
    {
        //TODO - should be a more efficient way of doing this than checking every frame
        if(tracks[currentTrack]->GetStatus() == sf::Sound::Stopped)
        {
            ++currentTrack;
            if(currentTrack >= tracks.size())
            {
                currentTrack = 0;
            }
            startCurrent();
        }
    }
}

/** \brief Initialise and load music files in the resource locations listed under "Music".
 *
*/
void MusicPlayer::load(const Ogre::String& path)
{
	if(!loaded)
	{

		std::cout << "Loading music..." << std::endl;

		//Get list of files in the resource.
		Ogre::StringVectorPtr musicFiles = Ogre::ResourceGroupManager
				::getSingleton().listResourceNames("Music");
		Ogre::StringVector::iterator it;
		tracks.reserve(musicFiles->size());

		//OgreOggSound::OgreOggSoundManager& soundmgr = OgreOggSound::OgreOggSoundManager::getSingleton();
		for(it = musicFiles->begin(); it != musicFiles->end(); ++it)
		{
		    std::cout << path << "/" << *it << std::endl;
			//Create sound objects for all files, Sound objects should be deleted automatically
			//by the sound manager.
			//TODO - check what this does if something goes wrong loading the file.
			//OgreOggSound::OgreOggISound* sound = soundmgr.createSound(*it, *it, true);// false, false, null));
		    Ogre::SharedPtr<sf::Music> track(new sf::Music());
		    //TODO - check for text encoding issues.
		    bool opened = track->OpenFromFile(path + "/" + *it);
			if(opened)
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
//
		if(tracks.size() == 0)
		{
			std::cerr << "No music files loaded... no music will be played" << std::endl;
		}
		else
		{
			//If there was any music loaded, store this.
			std::cout << "Loaded music" << std::endl;
			loaded = true;
		}


	}
}

/** \brief Start music playback if any music is loaded.
 *
*/
void MusicPlayer::start()
{
	if(loaded)
	{
		startCurrent();
	}
}

/** \brief Start music playback of current track.
 *
*/
void MusicPlayer::startCurrent()
{
	//tracks[currentTrack]->setListener(this);
	//tracks[currentTrack]->play();
    tracks[currentTrack]->Play();
}

/** \brief Callback function to start the next track.
 *
*/
/*
void MusicPlayer::soundStopped(OgreOggSound::OgreOggISound* sound)
{
	//Remove listener
	tracks[currentTrack]->setListener(static_cast<
			OgreOggSound::OgreOggISound::SoundListener*>(NULL));

	//Increment track number.
	++currentTrack;
	if(currentTrack >= tracks.size())
	{
		currentTrack = 0;
	}

	std::cout << "Starting next track" << std::endl;
	//Start
	startCurrent();
}
*/
