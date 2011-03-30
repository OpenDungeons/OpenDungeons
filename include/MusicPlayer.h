#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <SFML/Audio.hpp>
#include <OgreResourceGroupManager.h>
#include <OgreSingleton.h>
#include <OgreSharedPtr.h>
#include <vector>
#include <iostream>

/*! \brief Class to manage playing of music.
 *
 */
class MusicPlayer: public Ogre::Singleton<MusicPlayer>
{
    public:
        MusicPlayer();
        virtual ~MusicPlayer();
        void load(const Ogre::String& path);
        void update();
        void start();
        void stop();
        void next();

        static MusicPlayer& getSingleton();
        static MusicPlayer* getSingletonPtr();

    private:
        std::vector<Ogre::SharedPtr<sf::Music> > tracks;
        bool loaded;
        unsigned currentTrack;
};

#endif /* MUSICPLAYER_H */
