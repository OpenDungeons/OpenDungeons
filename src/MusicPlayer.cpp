#include "MusicPlayer.h"

/** \brief Initialise variables.
 *
*/
MusicPlayer::MusicPlayer() :
	loaded(false), currentTrack(0)
{

}

/*MusicPlayer::~MusicPlayer()
{
}*/

/** \brief Initialise and load music files in the resource locations listed under "Music".
 *
*/
void MusicPlayer::load()
{
/*	if(!loaded)
	{

		std::cout << "Loading music..." << std::endl;

		//Get list of files in the resource.
		Ogre::StringVectorPtr musicFiles = Ogre::ResourceGroupManager
				::getSingleton().listResourceNames("Music");
		Ogre::StringVector::iterator it;
		tracks.reserve(musicFiles->size());
		OgreOggSound::OgreOggSoundManager& soundmgr = OgreOggSound::OgreOggSoundManager::getSingleton();
		for(it = musicFiles->begin(); it != musicFiles->end(); ++it)
		{
			//Create sound objects for all files, Sound objects should be deleted automatically
			//by the sound manager.
			//TODO - check what this does if something goes wrong loading the file.
			OgreOggSound::OgreOggISound* sound = soundmgr.createSound(*it, *it, true);// false, false, null));
			if(sound)
			{
				tracks.push_back(sound);
				sound->disable3D(true); //Disable 3D sound for music files.
				//Stereo files are not positioned anyway, but in case we have mono music... this is necessary.

				//Lower volume to make it more in line with effects sounds.
				sound->setVolume(0.25);
			}

		}

		if(musicFiles->size() == 0)
		{
			std::cerr << "No music files found... no music will be played" << std::endl;
		}
		else
		{
			//If there was any music loaded, store this.
			std::cout << "Loaded music" << std::endl;
			loaded = true;
		}


	}*/
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
