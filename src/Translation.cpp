/*!
 * \file   Translation.cpp
 * \date   27 April 2011
 * \author StefanP.MUC
 * \brief  handles the translation
 */

#include "ResourceManager.h"

#include "Translation.h"

template<> Translation* Ogre::Singleton<Translation>::ms_Singleton = 0;

Translation::Translation()
{
#if defined(WIN32) && !defined(__CYGWIN__)
    setlocale(LC_CTYPE, "");
#else
    setlocale(LC_MESSAGES, "");
#endif

    ResourceManager* resMgr = ResourceManager::getSingletonPtr();

    std::string languagePath = resMgr->getLanguagePath();

    std::vector<std::string> fileList = resMgr->
            listAllFiles(languagePath);

    languageList.push_back("en");

    for(std::vector<std::string>::iterator itr = fileList.begin(),
            end = fileList.end(); itr != end; ++itr)
    {
        if((*itr).substr((*itr).size() - 3, 3) == ".po")
        {
            languageList.push_back(dictMgr.convertFilenameToLanguage(*itr));
        }
    }

    dictMgr.add_directory(languagePath);

    const char* lang = getenv("LANG");
    const char* language = getenv("LANGUAGE");

    if (language != NULL && strlen(language) > 0)
    {
        dictionary = dictMgr.get_dictionary(tinygettext::Language::from_env(language));
    }
    else if (lang != NULL && strlen(lang) > 0)
    {
        dictionary = dictMgr.get_dictionary(tinygettext::Language::from_env(lang));
    }
    else
    {
        dictionary = dictMgr.get_dictionary();
    }
}

Translation::~Translation()
{
}

/*! \brief Returns a reference to the singleton object
 *
 */
Translation& Translation::getSingleton()
{
    assert(ms_Singleton);
    return(*ms_Singleton);
}

/*! \brief Returns a pointer to the singleton object
 *
 */
Translation* Translation::getSingletonPtr()
{
    return ms_Singleton;
}
