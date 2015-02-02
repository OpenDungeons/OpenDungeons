/*!
 * \file   ResourceManager.cpp
 * \date   12 April 2011
 * \author StefanP.MUC
 * \brief  This class handles all the resources (pathes, files) needed by the
 *         sound and graphics facilities.
 *
 *  Copyright (C) 2011-2015  OpenDungeons Team
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

#include "utils/ResourceManager.h"

#include <OgreConfigFile.h>
#include <OgrePlatform.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#include <pwd.h> // getpwuid()
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#include <CoreFoundation/CoreFoundation.h>
#include <pwd.h> // getpwuid()
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <shlobj.h> //to get paths related functions
#ifndef PATH_MAX
    #define PATH_MAX _MAX_PATH   // redefine _MAX_PATH to be compatible with Darwin's PATH_MAX
#endif
#endif

#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <OgreString.h>
#include <OgreRenderTarget.h>

template<> ResourceManager* Ogre::Singleton<ResourceManager>::msSingleton = nullptr;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 && defined(OD_DEBUG)
//On windows, if the application is compiled in debug mode, use the plugins with debug prefix.
const std::string ResourceManager::PLUGINSCFG = "plugins_d.cfg";
#else
const std::string ResourceManager::PLUGINSCFG = "plugins.cfg";
#endif
const std::string ResourceManager::RESOURCECFG = "resources.cfg";
const std::string ResourceManager::MUSICSUBPATH = "music/";
const std::string ResourceManager::SOUNDSUBPATH = "sounds/";
const std::string ResourceManager::CONFIGSUBPATH = "config/";
const std::string ResourceManager::SCRIPTSUBPATH = "scripts/";
const std::string ResourceManager::LANGUAGESUBPATH = "lang/";
const std::string ResourceManager::SHADERCACHESUBPATH = "shaderCache/";
const std::string ResourceManager::CONFIGFILENAME = "ogre.cfg";
const std::string ResourceManager::LOGFILENAME = "opendungeons.log";
const std::string ResourceManager::CEGUILOGFILENAME = "CEGUI.log";

const std::string ResourceManager::RESOURCEGROUPMUSIC = "Music";
const std::string ResourceManager::RESOURCEGROUPSOUND = "Sound";

/*! \brief Initializes all paths and reads the ogre config file.
 *
 *  Provide a nice cross platform solution for locating the configuration
 *  files. On windows files are searched for in the current working
 *  directory, on OS X however you must provide the full path, the helper
 *  function macBundlePath does this for us.
 */
ResourceManager::ResourceManager() :
        mGameDataPath("./"),
        mUserDataPath("./"),
        mUserConfigPath("./")
{
    setupDataPath();
    setupUserDataFolders();
}

void ResourceManager::setupDataPath()
{
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    //TODO - Test osx support
    char applePath[1024];
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    assert(mainBundle);

    CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
    assert(mainBundleURL);

    CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
    assert(cfStringRef);

    CFStringGetCString(cfStringRef, applePath, 1024, kCFStringEncodingASCII);

    CFRelease(mainBundleURL);
    CFRelease(cfStringRef);

    mMacBundlePath = std::string(applePath + "/");

    mGameDataPath = mMacBundlePath + "Contents/Resources/";
#else // Windows and linux

    std::string path;
#ifdef OD_DATA_PATH
    path = std::string(OD_DATA_PATH);
#else
    path = ".";
#endif

    if(!path.empty())
    {
        mGameDataPath = path;
        if (*mGameDataPath.rbegin() != '/')
        {
            mGameDataPath.append("/");
        }
    }

    // Test whether there is data in "./" and remove the system path in that case.
    // Useful for developers.
    std::string resourceCfg = "./" + RESOURCECFG;
    if (boost::filesystem::exists(resourceCfg.c_str()))
    {
        // Don't warn on Windows as it is the default behaviour...
#if OGRE_PLATFORM != OGRE_PLATFORM_WIN32
        std::cout << "Note: Found data in the current folder. This data will be used instead of the installed one." << std::endl;
#endif
        mGameDataPath = "./";
    }
#endif

    std::cout << "Game data path is: " << mGameDataPath << std::endl;

#ifndef OGRE_STATIC_LIB
    mPluginsPath = mGameDataPath + PLUGINSCFG;
#endif
    mScriptPath = mGameDataPath + SCRIPTSUBPATH;
    mConfigPath = mGameDataPath + CONFIGSUBPATH;
    mSoundPath = mGameDataPath + SOUNDSUBPATH;
    mMusicPath = mGameDataPath + MUSICSUBPATH;
    mLanguagePath = mGameDataPath + LANGUAGESUBPATH;
}

void ResourceManager::setupUserDataFolders()
{
    // Empty the members so we can check their validity later.
    mUserDataPath.clear();
    mUserConfigPath.clear();

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    passwd* pw = getpwuid(getuid());
    if(pw)
    {
        mUserDataPath = std::string(pw->pw_dir) + "/Library/Application Support/opendungeons/";
        mUserConfigPath = mUserDataPath + "cfg/";
    }
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    // $XDG_DATA_HOME/opendungeons/
    // equals to: ~/.local/share/opendungeons/ most of the time
    if (std::getenv("XDG_DATA_HOME"))
    {
        mUserDataPath = std::string(std::getenv("XDG_DATA_HOME")) + "/opendungeons/";
    }
    else
    {
        // We create a sane default if possible: ~/.local/share/opendungeons
        passwd *pw = getpwuid(getuid());
        if(pw)
        {
            mUserDataPath = std::string(pw->pw_dir) + "/.local/share/opendungeons/";
        }
    }

    // $XDG_CONFIG_HOME/opendungeons
    // equals to: ~/.config/opendungeons/ most of the time
    if (std::getenv("XDG_CONFIG_HOME"))
    {
        mUserConfigPath = std::string(std::getenv("XDG_CONFIG_HOME")) + "/opendungeons/";
    }
    else
    {
        // We create a sane default if possible: ~/.config/opendungeons
        passwd *pw = getpwuid(getuid());
        if(pw)
        {
            mUserConfigPath = std::string(pw->pw_dir) + "/.config/opendungeons/";
        }
    }

#elif OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    char path[MAX_PATH];
    // %APPDATA% (%USERPROFILE%\Application Data)
    if(SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path)))
    {
        mUserDataPath = std::string(path) + "/opendungeons/";
        mUserConfigPath = mUserDataPath + "cfg/";
    }
#else
#error("Unknown platform!")
#endif

    // Use local defaults if everything else failed.
    if (mUserDataPath.empty())
        mUserDataPath = "./";
    if (mUserConfigPath.empty())
        mUserConfigPath = "./cfg/";

    try {
        boost::filesystem::create_directories(mUserDataPath);
    }
    catch (const boost::filesystem::filesystem_error& e) {
        //TODO - Exit gracefully
        std::cerr << "Fatal error creating user data folder: " << e.what() <<  std::endl;
        exit(1);
    }

    std::cout << "User data path is: " << mUserDataPath << std::endl;

    try {
        boost::filesystem::create_directories(mUserConfigPath);
    }
    catch (const boost::filesystem::filesystem_error& e) {
        //TODO - Exit gracefully
        std::cerr << "Fatal error creating user config folder: " << e.what() <<  std::endl;
        exit(1);
    }

    std::cout << "User config path is: " << mUserConfigPath << std::endl;

    try {
      boost::filesystem::create_directories(mUserDataPath.c_str() + SHADERCACHESUBPATH);
    }
    catch (const boost::filesystem::filesystem_error& e) {
        //TODO - Exit gracefully
        std::cerr << "Fatal error creating shader cache folder: " << e.what() <<  std::endl;
        exit(1);
    }

    mReplayPath = mUserDataPath + "replay/";
    try {
      boost::filesystem::create_directories(mReplayPath);
    }
    catch (const boost::filesystem::filesystem_error& e) {
        //TODO - Exit gracefully
        std::cerr << "Fatal error creating replay folder: " << e.what() <<  std::endl;
        exit(1);
    }

    mOgreCfgFile = mUserConfigPath + CONFIGFILENAME;
    mOgreLogFile = mUserDataPath + LOGFILENAME;
    mCeguiLogFile = mUserDataPath + CEGUILOGFILENAME;
    mShaderCachePath = mUserDataPath + SHADERCACHESUBPATH;
}

void ResourceManager::setupOgreResources()
{
    Ogre::ConfigFile cf;
    cf.load(mGameDataPath + RESOURCECFG);

    // Go through all sections & settings in the file
    Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

    Ogre::String secName = "";
    Ogre::String typeName = "";
    Ogre::String archName = "";
    while(seci.hasMoreElements())
    {
        secName = seci.peekNextKey();
        Ogre::ConfigFile::SettingsMultiMap* settings = seci.getNext();
        Ogre::ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i)
        {
            typeName = i->first;
            archName = mGameDataPath + i->second;
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
            // OS X does not set the working directory relative to the app,
            // In order to make things portable on OS X we need to provide
            // the loading with it's own bundle path location.
            // Unlike windows you can not rely on the curent working directory
            // for locating your configuration files and resources.

            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                    Ogre::String(std::string(mMacBundlePath) + archName), typeName, secName, true);
#else
            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                    archName, typeName, secName, true);
#endif
        }
    }
}

bool ResourceManager::hasFileEnding(const std::string& filename, const std::string& ending)
{
    return (filename.length() < ending.length())
            ? false
            : filename.compare(filename.length() - ending.length(),
                    ending.length(), ending) == 0;
}

std::vector<std::string> ResourceManager::listAllFiles(const std::string& directoryName)
{
    std::vector<std::string> files;
    using namespace boost::filesystem;

    if(is_directory(directoryName))
    {
        for(directory_iterator it(directoryName); it != directory_iterator(); ++it)
        {
            files.push_back(it->path().string());
        }
    }
    return files;
}

Ogre::StringVectorPtr ResourceManager::listAllMusicFiles()
{
    return Ogre::ResourceGroupManager::getSingleton().
            listResourceNames(RESOURCEGROUPMUSIC);
}

void ResourceManager::takeScreenshot(Ogre::RenderTarget* renderTarget)
{
    static int screenShotCounter = 0;

    static std::locale loc(std::wcout.getloc(), new boost::posix_time::time_facet("%Y-%m-%d_%H%M%S"));

    std::ostringstream ss;
    ss.imbue(loc);
    ss << "ODscreenshot_" << boost::posix_time::second_clock::local_time()
       << "_" << screenShotCounter++ << ".png";
    renderTarget->writeContentsToFile(getUserDataPath() + ss.str());
}
