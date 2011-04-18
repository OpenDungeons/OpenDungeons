/*! \file   ODApplication.cpp
 *  \author Ogre team, andrewbuck, oln, StefanP.MUC
 *  \date   07 April 2011
 *  \brief  Class ODApplication containing everything to start the game
 */

#ifndef ODAPPLICATION_H
#define ODAPPLICATION_H

#include <string>

#include <Ogre.h>

/*! Base class which manages the startup of OpenDungeons.
 */
class ODApplication : public Ogre::Singleton<ODApplication>
{
    public:
        ODApplication();
        static ODApplication& getSingleton();
        static ODApplication* getSingletonPtr();

        inline Ogre::Root* getRoot()const {return root;}
        inline Ogre::RenderWindow* getWindow()const {return window;}

        static const unsigned int PORT_NUMBER;
        static const double DEFAULT_FRAMES_PER_SECOND;
        static double MAX_FRAMES_PER_SECOND;
        static double turnsPerSecond;
        static const std::string VERSION;
        static const std::string VERSIONSTRING;
        static const std::string POINTER_INFO_STRING;
        static std::string MOTD;
        static const std::string HELP_MESSAGE;

    private:
        ODApplication(const ODApplication&);
        ~ODApplication();
        Ogre::Root* root;
        Ogre::RenderWindow* window;

        void cleanUp();
};

#endif

