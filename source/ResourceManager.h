/*!
 * \file   ResourceManager.h
 * \date   12 April 2011
 * \author StefanP.MUC
 * \brief  Header for the ResourceManager
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

#ifndef RESOURCEMANAGER_H_
#define RESOURCEMANAGER_H_

#include <string>

#include <OgreSingleton.h>
#include <OgreStringVector.h>

namespace Ogre {
  class RenderTarget;
}

class ResourceManager : public Ogre::Singleton<ResourceManager>
{
public:
    ResourceManager();
    ~ResourceManager()
    {}

    /*! \brief check if a filename has a specific extension
     *
     *  \param filename The filename, like "filename.ext"
     *  \param ending   The extension, like ".ext"
     *
     *  \return true or false depending if the filename has the extension or not
     */
    static bool hasFileEnding(const std::string& filename, const std::string& ending);

    void setupResources();

    /*! \brief gets all files within a directory
     *
     *  \param diretoryName the directory to scan for files
     *
     *  \return a vector with all file names
     */
    static std::vector<std::string> listAllFiles(const std::string& directoryName);

    /*! \brief returns all music files that Ogre knows of
     *
     *  \return a vector with all file names
     */
    Ogre::StringVectorPtr listAllMusicFiles();

    //! \brief saves a screenshot
    void takeScreenshot(Ogre::RenderTarget* renderTarget);

    inline const std::string& getResourcePath() const
    { return mResourcePath; }

    inline const std::string& getHomePath() const
    { return mHomePath; }

    inline const std::string& getPluginsPath() const
    { return mPluginsPath; }

    inline const std::string& getMusicPath() const
    { return mMusicPath; }

    inline const std::string& getScriptPath() const
    { return mScriptPath; }

    inline const std::string& getSoundPath() const
    { return mSoundPath; }

    inline const std::string& getLanguagePath() const
    { return mLanguagePath; }

    inline const std::string& getShaderCachePath() const
    { return mShaderCachePath; }

    inline const std::string& getCfgFile() const
    { return mOgreCfgFile; }

    inline const std::string& getLogFile() const
    { return mOgreLogFile; }

protected:
    static std::string locateHomeFolder();

private:
    std::string mResourcePath;
    std::string mHomePath;
    std::string mPluginsPath;
    std::string mMusicPath;
    std::string mSoundPath;
    std::string mScriptPath;
    std::string mLanguagePath;
    std::string mMacBundlePath;
    std::string mShaderCachePath;
    std::string mOgreCfgFile;
    std::string mOgreLogFile;

    static const std::string PLUGINSCFG;
    static const std::string RESOURCECFG;
    static const std::string MUSICSUBPATH;
    static const std::string SOUNDSUBPATH;
    static const std::string SCRIPTSUBPATH;
    static const std::string LANGUAGESUBPATH;
    static const std::string SHADERCACHESUBPATH;
    static const std::string CONFIGFILENAME;
    static const std::string LOGFILENAME;

    static const std::string RESOURCEGROUPMUSIC;
    static const std::string RESOURCEGROUPSOUND;
};

#endif // RESOURCEMANAGER_H_
