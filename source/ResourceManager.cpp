/*!
 * \file   ResourceManager.cpp
 * \date   12 April 2011
 * \author StefanP.MUC
 * \brief  This class handles all the resources (pathes, files) needed by the
 *         sound and graphics facilities.
 *
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

#include <cstdlib>
#include <ctime>

#include <dirent.h>
#include <sys/stat.h>
#include <OgreConfigFile.h>

#include <OgrePlatform.h>
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <userenv.h>
#include <direct.h>
#include <errno.h>

#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#include <CoreFoundation/CoreFoundation.h>

#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#include <cerrno>
#include <cstring>
#endif

#include <OgreString.h>
#include <OgreRenderTarget.h>

#include "ResourceManager.h"

template<> ResourceManager* Ogre::Singleton<ResourceManager>::msSingleton = 0;
#if defined(OD_DEBUG)
//On windows, if the application is compiled in debug mode, use the plugins with debug prefix.
const std::string ResourceManager::PLUGINSCFG = "plugins_d.cfg";
#else
const std::string ResourceManager::PLUGINSCFG = "plugins.cfg";
#endif
const std::string ResourceManager::RESOURCECFG = "resources.cfg";
const std::string ResourceManager::MUSICSUBPATH = "music/";
const std::string ResourceManager::SOUNDSUBPATH = "sounds/";
const std::string ResourceManager::SCRIPTSUBPATH = "scripts/";
const std::string ResourceManager::LANGUAGESUBPATH = "lang/";
const std::string ResourceManager::SHADERCACHESUBPATH = "shaderCache/";
const std::string ResourceManager::CONFIGFILENAME = "ogre.cfg";
const std::string ResourceManager::LOGFILENAME = "opendungeons.log";

const std::string ResourceManager::RESOURCEGROUPMUSIC = "Music";
const std::string ResourceManager::RESOURCEGROUPSOUND = "Sound";

bool ResourceManager::hasFileEnding(const std::string& filename, const std::string& ending)
{
    return (filename.length() < ending.length())
            ? false
            : filename.compare(filename.length() - ending.length(),
                    ending.length(), ending) == 0;
}

/* TODO: insert some general easy-access functions that do all the
 * OgreResourceManager calls (e.g. returning all sound and music names/files)
 */

/*! \brief Initializes all paths and reads the ogre config file.
 *
 *  Provide a nice cross platform solution for locating the configuration
 *  files. On windows files are searched for in the current working
 *  directory, on OS X however you must provide the full path, the helper
 *  function macBundlePath does this for us.
 */
ResourceManager::ResourceManager() :
        mResourcePath("./"),
        mHomePath("./"),
        mPluginsPath(""),
        mMusicPath(""),
        mSoundPath(""),
        mScriptPath(""),
        mLanguagePath(""),
        mMacBundlePath(""),
        mOgreCfgFile(""),
        mOgreLogFile("")
{
    bool success = false;
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    //TODO - osx support
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

    mResourcePath = macBundlePath + "Contents/Resources/";
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX

    // getenv return value should not be touched/freed.
    char* path = std::getenv("OPENDUNGEONS_DATA_PATH");
    if(path != 0)
    {
        mResourcePath = path;
        if (*mResourcePath.end() != '/')
        {
            mResourcePath.append("/");
        }
    }
#endif

    /* If variable is not set, assume we are in a build dir and
     * use the current dir for config files.
     */
#if (OGRE_PLATFORM == OGRE_PLATFORM_LINUX)
    char* useHomeDir = std::getenv("OPENDUNGEONS_DATA_PATH");
    if (useHomeDir != 0)
    {
        mHomePath = locateHomeFolder() + "/.OpenDungeons/";
#else
        mHomePath = locateHomeFolder() + "\\OpenDungeons\\";
#endif


        success = createFolderIfNotExists(mHomePath);
        if(!success)
        {
            //TODO - Exit gracefully
            std::cerr << "Fatal error creating game storage folder" << std::endl;
            exit(1);
        }
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    }
#endif

    std::cout << "Home path is: " << mHomePath << std::endl;

    //Create shader cache folder.
    success = createFolderIfNotExists(mHomePath.c_str() + SHADERCACHESUBPATH);
    if(!success)
    {
        std::cerr << "Fatal error creating shader cache folder" << std::endl;
        exit(1);
    }

#ifndef OGRE_STATIC_LIB
    mPluginsPath = mResourcePath + PLUGINSCFG;
#endif

    mOgreCfgFile = mHomePath + CONFIGFILENAME;
    mOgreLogFile = mHomePath + LOGFILENAME;
    mScriptPath = mResourcePath + SCRIPTSUBPATH;
    mSoundPath = mResourcePath + SOUNDSUBPATH;
    mMusicPath = mResourcePath + MUSICSUBPATH;
    mLanguagePath = mResourcePath + LANGUAGESUBPATH;
    mShaderCachePath = mHomePath + SHADERCACHESUBPATH;

    //Make shader cache folder if it does not exist.
}

void ResourceManager::setupResources()
{
    Ogre::ConfigFile cf;
    cf.load(mResourcePath + RESOURCECFG);

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
            archName = mResourcePath + i->second;
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

std::vector<std::string> ResourceManager::listAllFiles(const std::string& directoryName)
{
    std::vector<std::string> files;

    DIR* dir = opendir(directoryName.c_str());
    if(dir)
    {
        struct dirent* dp;
        while((dp = readdir(dir)) != 0)
        {
            files.push_back(dp->d_name);
        }
        closedir(dir);
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
    //FIXME: Could use normal date formatting here instead of time value
    const std::time_t time = std::time(nullptr);
    std::ostringstream ss;
    ss << "ODscreenshot_" << time << ".png";
    renderTarget->writeContentsToFile(getHomePath() + ss.str());
}

bool ResourceManager::createFolderIfNotExists(const std::string& name)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
//Not implemented. Can probably use the same code as linux.
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    struct stat statbuf;
    if (stat(name.c_str(), &statbuf) != 0)
    {
        if(errno == ENOENT)
        {
            mkdir(name.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            return true;
        }
        else
        {
            std::cerr << "Error reading directory: " << strerror(errno) << std::endl;
            return false;
        }
    }
    //Directory exists.
    return true;
#elif OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    struct stat statBuf;
    int result;

    result = stat(name.c_str(), &statBuf);
    if(result == 0)
    {
        //exists
        if(statBuf.st_mode & _S_IFREG)
        {
            //.OpenDungeons is a file and not a directory, bail out.
            std::cerr << "Error: \"" << name << "\" is a file" << std::endl;
            return false;
        }
    }
    else
    {
        //does not exist or inaccessible
        switch(errno)
        {
        case ENOENT:
            {
                int dirCreated = ::_mkdir(name.c_str());
                if(dirCreated != 0 && errno != EEXIST)
                {
                    //FIXME: Handle this properly.
                    std::cerr << "Failed create subdirectory in home directory (" << name << ") !" << std::endl;
                    return false;
                }

                break;
            }
        case EINVAL:
            {
                std::cerr << "Invalid parameter to stat()!" << std::endl;
                return false;
            }
        default:
            {
                std::cerr << "Unexpected error in stat()!" << std::endl;
                return false;
            }
        }
    }
    return true;
#endif //OGRE_PLATFORM
}

std::string ResourceManager::locateHomeFolder()
{
    std::string homeFolderPath;
    char* path = 0;
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
//Not implemented. Can probably use the same code as linux.
return ".";
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
//On linux and similar, use home dir
    //http://linux.die.net/man/3/getpwuid
    path = std::getenv("HOME");
    homeFolderPath = (path != 0)
                ? path
                : "./";
#elif OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    path = std::getenv("USERPROFILE");
    if(path == 0)
    {
        path = std::getenv("HOMEDRIVE");
        char* pathb = std::getenv("HOMEPATH");
        if(path == 0 || pathb == 0)
        {
            path = std::getenv("HOME");
            homeFolderPath = (path != 0)
                ? path
                : "./";
        }
        else
        {
            homeFolderPath = std::string(path) + pathb;
        }
    }
    else
    {
        homeFolderPath = path;
    }
#else
#error("Unknown platform!")
#endif

    return homeFolderPath;
}
