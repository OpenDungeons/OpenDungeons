/*!
 * \file   ResourceManager.cpp
 * \date   12 April 2011
 * \author StefanP.MUC
 * \brief  This class handles all the resources (pathes, files) needed by the
 *         sound and graphics facilities.
 */

#include <cstdlib>

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
#include <cstdlib>
#endif

#include "ODApplication.h"

#include "ResourceManager.h"

template<> ResourceManager* Ogre::Singleton<ResourceManager>::ms_Singleton = 0;

const std::string ResourceManager::PLUGINSCFG = "plugins.cfg";
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

/*! \brief check if a filename has a specific extension
 *
 *  \param filename The filename, like "filename.ext"
 *  \param ending   The extension, like ".ext"
 *
 *  \return true or false depending if the filename has the extension or not
 */
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
        screenshotCounter(0),
        resourcePath("./"),
        homePath("./"),
        pluginsPath(""),
        musicPath(""),
        soundPath(""),
        scriptPath(""),
        languagePath(""),
        macBundlePath(""),
        ogreCfgFile(""),
        ogreLogFile("")
{
    bool success = false;
//FIXME - we should check that there is no _file_ with the same name as the dir we want to use.
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
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

    macBundlePath = std::string(applePath + "/");

    resourcePath = macBundlePath + "Contents/Resources/";
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX

    // getenv return value should not be touched/freed.
    char* path = std::getenv("OPENDUNGEONS_DATA_PATH");
    if(path != 0)
    {
        resourcePath = path;
        if (*resourcePath.end() != '/')
        {
            resourcePath.append("/");
        }
    }

    /* If variable is not set, assume we are in a build dir and
     * use the current dir for config files.
     */
    char* useHomeDir = std::getenv("OPENDUNGEONS_DATA_PATH");
    if (useHomeDir != 0)
    {
        //On linux and similar, use home dir
        //http://linux.die.net/man/3/getpwuid
        char* path = std::getenv("HOME");
        homePath = (path != 0)
                ? path
                : ".";

        homePath.append("/.OpenDungeons");

        success = createFolderIfNotExists(homePath);
        if(!success)
        {
            std::cerr << "Fatal error in folder setup, exiting" << std::endl;
            exit(1);
        }

        homePath.append("/");

        
    }

#elif OGRE_PLATFORM == OGRE_PLATFORM_WIN32

/*
*Based on homePath in Qt (http://qt.gitorious.org/qt/qt/blobs/4.7/src/corelib/io/qfsfileengine_win.cpp) 
*FIXME: This might not work properly if the path contains non-ansi characters.
*/
	std::cout << "Trying to find home dir" << std::endl;
	std::string homeDirectoryString;
	//Get process handle
	HANDLE procHandle = ::GetCurrentProcess();
	HANDLE token = 0;
	//Get process token
	BOOL ok = ::OpenProcessToken(procHandle, TOKEN_QUERY, &token);
	if(ok)
	{
		DWORD wBufferSize = 0;
		//Get buffer size needed for pathname
		ok = ::GetUserProfileDirectoryW(token, NULL, &wBufferSize);
		if(!ok && wBufferSize)
		{
			LPWSTR homeDirWBuffer = new WCHAR[wBufferSize];
			ok = ::GetUserProfileDirectoryW(token, homeDirWBuffer, &wBufferSize);
			if(ok)
			{
				
				int bufferSize = ::WideCharToMultiByte(CP_UTF8, 0,
					homeDirWBuffer, wBufferSize, NULL, 0, NULL, NULL);
				LPSTR homeDirBuffer = new CHAR[bufferSize + 1];
				int convOk = ::WideCharToMultiByte(CP_UTF8, 0,
					homeDirWBuffer, wBufferSize, homeDirBuffer, bufferSize, NULL, NULL);
				homeDirBuffer[bufferSize] = '\0'; //Append null terminator.
				if(convOk)
				{
					homeDirectoryString.append(homeDirBuffer);
				}
			}
		}
		std::cout << "Found using getuserprofiledirectory: " << homeDirectoryString << std::endl;
		
	}
	if(homeDirectoryString.empty())
	{
		//char* envString = 0;
		//homeDirectoryString = std::string(std::getenv("USERPROFILE"));
		/*if(homeDirectoryString.empty)
		{
			homeDirectoryString = std::string(std::getenv("HOMEDRIVE")) + std::getenv("HOMEPATH");
			if(homeDirectoryString.empty)
			{
				homeDirectoryString = std::getenv("HOME");
			}
		}*/
		//If all this fails we use the current dir.
		homeDirectoryString = ".";
	}
	homePath = homeDirectoryString + "\\.OpenDungeons";
	
	std::cout << "Home path is: " << homePath << std::endl;

    success = createFolderIfNotExists(homePath);
    if(!success)
    {
        std::cerr << "Fatal error in folder setup, exiting" << std::endl;
    }
	
	//Set line endings to unix-style for consistency.
	std::replace(homePath.begin(), homePath.end(), '\\', '/');
	homePath += "/";

#endif

    //Create shader cache folder.
    success = createFolderIfNotExists(homePath.c_str() + SHADERCACHESUBPATH);
    if(!success)
    {
        std::cerr << "Fatal error in folder setup, exiting" << std::endl;
        exit(1);
    }

#ifndef OGRE_STATIC_LIB
    pluginsPath = resourcePath + PLUGINSCFG;
#endif

    ogreCfgFile = homePath + CONFIGFILENAME;
    ogreLogFile = homePath + LOGFILENAME;
    scriptPath = resourcePath + SCRIPTSUBPATH;
    soundPath = resourcePath + SOUNDSUBPATH;
    musicPath = resourcePath + MUSICSUBPATH;
    languagePath = resourcePath + LANGUAGESUBPATH;
    shaderCachePath = homePath + SHADERCACHESUBPATH;

    //Make shader cache folder if it does not exist.
    
}

void ResourceManager::setupResources()
{
    Ogre::ConfigFile cf;
    cf.load(resourcePath + RESOURCECFG);

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
            archName = resourcePath + i->second;
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
            // OS X does not set the working directory relative to the app,
            // In order to make things portable on OS X we need to provide
            // the loading with it's own bundle path location.
            // Unlike windows you can not rely on the curent working directory
            // for locating your configuration files and resources.

            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                    Ogre::String(std::string(macBundlePath) + archName), typeName, secName, true);
#else
            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                    archName, typeName, secName, true);
#endif
        }
    }
}

/*! \brief gets all files within a directory
 *
 *  \param diretoryName the directory to scan for files
 *
 *  \return a vector with all file names
 */
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

/*! \brief returns all music files that Ogre knows of
 *
 *  \return a vector with all file names
 */
Ogre::StringVectorPtr ResourceManager::listAllMusicFiles()
{
    return Ogre::ResourceGroupManager::getSingleton().
            listResourceNames(RESOURCEGROUPMUSIC);
}

/*! \brief saves a screenshot
 *
 */
void ResourceManager::takeScreenshot()
{
    //FIXME: the counter is reset after each start, this overwrites existing pictures
    std::ostringstream ss;
    ss << "ODscreenshot_" << ++screenshotCounter << ".png";
    ODApplication::getSingleton().getWindow()->writeContentsToFile(getHomePath() + ss.str());
}

/*! \brief Creates a folder if it doesn't already exist.
 *
 */
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
                if(dirCreated != 0)
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
