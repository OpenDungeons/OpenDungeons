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

        static const unsigned int PORT_NUMBER;
        static const double BLENDER_UNITS_PER_OGRE_UNIT;
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
        Ogre::Root* mRoot;
        Ogre::Camera* mCamera;
        Ogre::RenderWindow* mWindow;
        std::string mHomePath;
        std::string mResourcePath;

        bool setup();
        void createViewports();
        void setupResources();
        void createCamera();
        void createScene();
        std::string getHomePath();
        std::string macBundlePath();
};

#endif

