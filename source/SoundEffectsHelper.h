/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#ifndef SOUNDEFFECTSHELPER_H_
#define SOUNDEFFECTSHELPER_H_

#include <OgreSingleton.h>
#include <OgreQuaternion.h>
#include <OgreVector3.h>
#include <SFML/Audio.hpp>
#include <vector>
#include <OgreSharedPtr.h>

//Forward declarations
class CreatureDefinition;
class CreatureSound;

//! \brief Helper class to manage sound effects.
class SoundEffectsHelper: public Ogre::Singleton<SoundEffectsHelper>
{
    public:

        typedef std::vector<sf::Sound> SoundFXVector;

        enum InterfaceSound
        {
            BUTTONCLICK, DIGSELECT, PICKUP,
            //NOTE Pickup and Drop will be added to individual creatures later
            //, though may still be used for items or something
            DROP,
            BUILDROOM,
            BUILDTRAP,
            CLAIM,
            NUM_INTERFACE_SOUNDS
        };

        SoundEffectsHelper();
        virtual ~SoundEffectsHelper();

        void initialiseSound(Ogre::String soundFolderPath);
        void setListenerPosition(const Ogre::Vector3& position,
                const Ogre::Quaternion& orientation);//, const Ogre::Vector3& velocity = Ogre::Vector3::ZERO);
        void playBlockDestroySound(int tileX, int tileY);
        void playInterfaceSound(InterfaceSound sound, bool stopCurrent = true);
        void playInterfaceSound(InterfaceSound sound,
                const Ogre::Vector3& position, bool stopCurrent = true);
        void playInterfaceSound(InterfaceSound sound, int tileX, int tileY,
                bool stopCurrent = true);

        void registerCreatureClass(const std::string& className);
        //TODO check if we should keep references to creature sounds here or not. Could create problems on shutdown.
        Ogre::SharedPtr<CreatureSound> createCreatureSound(
                const std::string& className);
    private:

        typedef std::vector<Ogre::SharedPtr<sf::SoundBuffer> >
                SoundFXBufferVector;

        //List of sounds for block getting dug out
        SoundFXVector digSounds;
        SoundFXBufferVector digSoundBuffers;
        //Next dig sound to be played
        unsigned nextDigSound;

        //Interface sounds, such as clicks
        SoundFXVector interfaceSounds;
        SoundFXBufferVector interfaceSoundBuffers;

        std::map<std::string, SoundFXBufferVector> creatureSoundBuffers;
};

#endif // SOUNDEFFECTSHELPER_H_