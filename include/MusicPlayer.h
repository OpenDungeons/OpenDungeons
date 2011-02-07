#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

//#include <OgreOggSoundManager.h>
//#include <OgreOggISound.h>
#include <SFML/Audio.hpp>
#include <OgreResourceGroupManager.h>
#include <OgreSingleton.h>
#include <OgreSharedPtr.h>
#include <vector>
#include <iostream>

/*! \brief Class to manage playing of music.
 *
 */
class MusicPlayer :
    public Ogre::Singleton<MusicPlayer>
{ // : public OgreOggSound::OgreOggISound::SoundListener {
public:
	MusicPlayer();
	virtual ~MusicPlayer();
	void update();
	void load(const Ogre::String& path);
	void start();
	void stop();
	void next();

	static MusicPlayer& getSingleton();
	static MusicPlayer* getSingletonPtr();

private:
	void startCurrent();
	//void soundStopped(OgreOggSound::OgreOggISound* sound);

	std::vector<Ogre::SharedPtr<sf::Music> > tracks;
	bool loaded;
	unsigned currentTrack;
};

#endif /* MUSICPLAYER_H */
