#ifndef OPENDUNGEONSAPPLICATION_H
#define OPENDUNGEONSAPPLICATION_H

#include <string>

#include <Ogre.h>

/*! Base class which manages the startup of OpenDungeons.
 */
class OpenDungeonsApplication
{
    public:
        OpenDungeonsApplication();
        ~OpenDungeonsApplication();
        void go();

    private:
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

