/*!
 * \file   Translation.cpp
 * \date   27 April 2011
 * \author StefanP.MUC
 * \brief  handles the translation
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

#include "ResourceManager.h"

#include "Translation.h"

template<> Translation* Ogre::Singleton<Translation>::msSingleton = 0;

Translation::Translation()
{
#if defined(WIN32) && !defined(__CYGWIN__)
    setlocale(LC_CTYPE, "");
#else
    setlocale(LC_MESSAGES, "");
#endif

    ResourceManager* resMgr = ResourceManager::getSingletonPtr();

    std::string languagePath = resMgr->getLanguagePath();
    std::vector<std::string> fileList = resMgr->listAllFiles(languagePath);

    //tell the dictionary manager where to find the .po files
    mDictMgr.add_directory(languagePath);

    //we have no .po file for en (default, hardcoded) so we add it manually
    mLanguageList.push_back("en");
    mLanguageNiceNames.push_back(tinygettext::Language::from_name("en") + " (en)");

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
            currentLang = mDictMgr.convertFilenameToLanguage(*itr);
            mLanguageList.push_back(currentLang);
            mLanguageNiceNames.push_back(
                            tinygettext::Language::from_name(currentLang) +
                            " (" + currentLang + ")");
        }
    }

    //TODO: after we have a user cfg file, load his language at game start
    mDictionary = mDictMgr.get_dictionary();

    /* this is how STK does it but I think we won't need to hack
     * us through env vars after we have a config file
    const char* language = getenv("LANGUAGE");
    mDictionary = (language != NULL && strlen(language) > 0)
            ? mDictMgr.get_dictionary(tinygettext::Language::from_env(language))
            : mDictMgr.get_dictionary();
     */
}

Translation::~Translation()
{
}
