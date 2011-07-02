/*!
 * \file   ResourceManager.h
 * \date   12 April 2011
 * \author StefanP.MUC
 * \brief  Header for the ResourceManager
 */

#ifndef RESOURCEMANAGER_H_
#define RESOURCEMANAGER_H_

#include <string>

#include <Ogre.h>

class ResourceManager : public Ogre::Singleton<ResourceManager>
{
    public:
        ResourceManager();
        static ResourceManager& getSingleton();
        static ResourceManager* getSingletonPtr();

        static bool hasFileEnding(const std::string& filename, const std::string& ending);

        void setupResources();
        std::vector<std::string> listAllFiles(const std::string& directoryName);
        Ogre::StringVectorPtr listAllMusicFiles();
        void takeScreenshot();

        inline const std::string& getResourcePath() const{return resourcePath;}
        inline const std::string& getHomePath() const{return homePath;}
        inline const std::string& getPluginsPath() const{return pluginsPath;}
        inline const std::string& getMusicPath() const{return musicPath;}
        inline const std::string& getSoundPath() const{return soundPath;}
        inline const std::string& getLanguagePath() const{return languagePath;}
        inline const std::string& getCfgFile() const{return ogreCfgFile;}
        inline const std::string& getLogFile() const{return ogreLogFile;}

    private:
        ResourceManager(const ResourceManager&);
        ~ResourceManager();

        unsigned int screenshotCounter;
        std::string resourcePath;
        std::string homePath;
        std::string pluginsPath;
        std::string musicPath;
        std::string soundPath;
        std::string languagePath;
        std::string macBundlePath;
        std::string ogreCfgFile;
        std::string ogreLogFile;

        static const std::string PLUGINSCFG;
        static const std::string RESOURCECFG;
        static const std::string MUSICSUBPATH;
        static const std::string SOUNDSUBPATH;
        static const std::string LANGUAGESUBPATH;
        static const std::string CONFIGFILENAME;
        static const std::string LOGFILENAME;

        static const std::string RESOURCEGROUPMUSIC;
        static const std::string RESOURCEGROUPSOUND;
};

#endif /* RESOURCEMANAGER_H_ */
