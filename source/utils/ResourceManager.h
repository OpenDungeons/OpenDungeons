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

    //! \brief Initializes the Ogre resources path
    //! \note Used after the Ogre Root initialization
    void setupOgreResources();

    /*! \brief check if a filename has a specific extension
     *  \param filename The filename, like "filename.ext"
     *  \param ending   The extension, like ".ext"
     *  \return true or false depending if the filename has the extension or not
     */
    static bool hasFileEnding(const std::string& filename, const std::string& ending);

    /*! \brief gets all files within a directory
     *  \param directoryName the directory to scan for files
     *  \return a vector with all file names
     */
    static std::vector<std::string> listAllFiles(const std::string& directoryName);

    /*! \brief returns all music files that Ogre knows of
     *  \return a vector with all file names
     */
    Ogre::StringVectorPtr listAllMusicFiles();

    //! \brief saves a screenshot
    void takeScreenshot(Ogre::RenderTarget* renderTarget);

    inline const std::string& getGameDataPath() const
    { return mGameDataPath; }

    inline const std::string& getUserDataPath() const
    { return mUserDataPath; }

    inline const std::string& getUserConfigPath() const
    { return mUserConfigPath; }

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

private:
    //! \brief The application data path
    //! \example "/usr/share/game/opendungeons" on linux
    //! \example "C:/opendungeons" on windows
    std::string mGameDataPath;

    //! \brief Specific mac bundle path
    std::string mMacBundlePath;

    //! \brief The user custom data path
    //! \example "~/.local/share/opendungeons" on linux
    //! \example %APPDATA% "%USERPROFILE%\Application Data\opendungeons" on windows
    std::string mUserDataPath;

    //! \brief The user custom config path
    //! \example "~/.config/opendungeons" on linux
    //! Same as home path + "cfg/" on Windows.
    std::string mUserConfigPath;

    //! \brief The Ogre config file
    std::string mOgreCfgFile;

    //! \brief Main files in the user data path
    std::string mOgreLogFile;
    std::string mShaderCachePath;

    //! \brief Specific data sub-paths.
    std::string mPluginsPath;
    std::string mMusicPath;
    std::string mSoundPath;
    std::string mScriptPath;
    std::string mLanguagePath;

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

    //! \brief Setup user data and config path
    void setupUserDataFolders();

    //! \brief Setup the game data path
    //! \note If game data path is found in the current folder,
    //! then the local data path will be used.
    void setupDataPath();
};

#endif // RESOURCEMANAGER_H_
