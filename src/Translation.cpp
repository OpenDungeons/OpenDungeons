/*!
 * \file   Translation.cpp
 * \date   27 April 2011
 * \author StefanP.MUC
 * \brief  handles the translation
 */

#include "ResourceManager.h"

#include "Translation.h"

template<> Translation* Ogre::Singleton<Translation>::ms_Singleton = 0;

/*! \brief Initializes tinygettext (loading available languages)
 *
 */
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

    //tell the dictionary manager where to find the .po files
    dictMgr.add_directory(languagePath);

    //we have no .po file for en (default, hardcoded) so we add it manually
    languageList.push_back("en");
    languageNiceNames.push_back(tinygettext::Language::from_name("en") + " (en)");

    /* check what .po files are available and put them into a list
     * and initializes the nice names list in the style of:
     * English (en)
     * French (fr)
     * German (de)
     */
    std::string currentLang;
    for(std::vector<std::string>::iterator itr = fileList.begin(),
            end = fileList.end(); itr != end; ++itr)
    {
        //only load .po files
        if(ResourceManager::hasFileEnding(*itr, ".po"))
        {
            currentLang = dictMgr.convertFilenameToLanguage(*itr);
            languageList.push_back(currentLang);
            languageNiceNames.push_back(
                            tinygettext::Language::from_name(currentLang) +
                            " (" + currentLang + ")");
        }
    }

    //try to set the users system language
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
