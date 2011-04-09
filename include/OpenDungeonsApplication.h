#ifndef OPENDUNGEONSAPPLICATION_H
#define OPENDUNGEONSAPPLICATION_H

#include <string>

#include <Ogre.h>

/*! Base class which manages the startup of OpenDungeons.
 */
class OpenDungeonsApplication : public Ogre::Singleton<OpenDungeonsApplication>
{
    public:
        OpenDungeonsApplication();
        static OpenDungeonsApplication& getSingleton();
        static OpenDungeonsApplication* getSingletonPtr();

        static const unsigned int PORT_NUMBER;
        static const double BLENDER_UNITS_PER_OGRE_UNIT;
        static const double DEFAULT_FRAMES_PER_SECOND;
        static const std::string VERSION;
        static const std::string POINTER_INFO_STRING;
        static const std::string HELP_MESSAGE;

    private:
        OpenDungeonsApplication(const OpenDungeonsApplication&);
        ~OpenDungeonsApplication();
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

