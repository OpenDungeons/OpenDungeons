#ifndef OPENDUNGEONSAPPLICATION_H
#define OPENDUNGEONSAPPLICATION_H

#include <string>
#include <sstream>
#include <fstream>

#include <OIS.h>
#include <Ogre.h>
#include <OgreConfigFile.h>
#include <sys/stat.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <shlwapi.h>
#include <direct.h>
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#include <CoreFoundation/CoreFoundation.h>

// This function will locate the path to our application on OS X,
// unlike windows you can not rely on the curent working directory
// for locating your configuration files and resources.
std::string macBundlePath()
{
    char path[1024];
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    assert(mainBundle);

    CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
    assert(mainBundleURL);

    CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
    assert(cfStringRef);

    CFStringGetCString(cfStringRef, path, 1024, kCFStringEncodingASCII);

    CFRelease(mainBundleURL);
    CFRelease(cfStringRef);

    return std::string(path);
}
#endif

#include "Globals.h"
#include "ExampleFrameListener.h"
#include "Tile.h"
#include "MusicPlayer.h"
#include "SoundEffectsHelper.h"

/*! Base class which manages the startup of OpenDungeons.
 */
class OpenDungeonsApplication
{
    public:
        OpenDungeonsApplication();
        ~OpenDungeonsApplication();
        void go(void);
        void createCamera(void);
        void createScene(void);
        void createFrameListener(void);

    private:
        Ogre::Root* mRoot;
        Ogre::Camera* mCamera;
        ExampleFrameListener* mFrameListener;
        Ogre::RenderWindow* mWindow;
        Ogre::String mResourcePath;
        Ogre::String mHomePath;

        bool setup(void);
        bool configure(void);
        void chooseSceneManager(void);
        void createViewports(void);
        void setupResources(void);
        void loadResources(void);
        Ogre::String getHomePath();
};

#endif

