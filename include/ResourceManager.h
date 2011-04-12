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

        void setupResources();

        inline const std::string& getResourcePath() const{return resourcePath;}
        inline const std::string& getHomePath() const{return homePath;}
        inline const std::string& getPluginsPath() const{return pluginsPath;}
        inline const std::string& getMusicPath() const{return musicPath;}
        inline const std::string& getSoundPath() const{return soundPath;}

    private:
        ResourceManager(const ResourceManager&);
        ~ResourceManager();

        std::string resourcePath;
        std::string homePath;
        std::string pluginsPath;
        std::string musicPath;
        std::string soundPath;
        std::string macBundlePath;

        static const std::string PLUGINSCFG;
        static const std::string RESOURCECFG;
        static const std::string MUSICSUBPATH;
        static const std::string SOUNDSUBPATH;
};

#endif /* RESOURCEMANAGER_H_ */
