//  tinygettext - A gettext replacement that works directly on .po files
//  Copyright (C) 2006 Ingo Ruhnke <grumbel@gmx.de>
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "po_parser.hpp"
#include "ResourceManager.h"
#include "LogManager.h"

#include "dictionary_manager.hpp"

namespace tinygettext {

DictionaryManager::DictionaryManager(const std::string& charset_) :
          dictionaries(),
          search_path(),
          charset(charset_),
          use_fuzzy(true),
          current_language(),
          current_dict(0),
          empty_dict()
{
}

DictionaryManager::~DictionaryManager()
{
    for(Dictionaries::iterator i = dictionaries.begin(); i != dictionaries.end(); ++i)
    {
        delete i->second;
    }
}

void DictionaryManager::clear_cache()
{
    for(Dictionaries::iterator i = dictionaries.begin(); i != dictionaries.end(); ++i)
    {
        delete i->second;
    }
    dictionaries.clear();

    current_dict = 0;
}

Dictionary& DictionaryManager::get_dictionary()
{
    if (current_dict)
    {
        return *current_dict;
    }
    else
    {
        if (current_language)
        {
            current_dict = &get_dictionary(current_language);
            return *current_dict;
        }
        else
        {
            return empty_dict;
        }
    }
}

Dictionary& DictionaryManager::get_dictionary(const Language& language)
{
    assert(language);

    Dictionaries::iterator i = dictionaries.find(language);
    if (i != dictionaries.end())
    {
        return *i->second;
    }
    else // Dictionary for languages lang isn't loaded, so we load it
    {
        Dictionary* dict = new Dictionary(charset);
        LogManager& logMgr = LogManager::getSingleton();

        dictionaries[language] = dict;

        ResourceManager& resMgr = ResourceManager::getSingleton();
        for (SearchPath::reverse_iterator p = search_path.rbegin(); p != search_path.rend(); ++p)
        {
            std::vector<std::string> files = resMgr.listAllFiles(*p);

            std::string best_filename;
            int best_score = 0;

            for(std::vector<std::string>::iterator filename = files.begin(); filename != files.end(); ++filename)
            {
                // check if filename matches requested language
                if (ResourceManager::hasFileEnding(*filename, ".po"))
                { // ignore anything that isn't a .po file
                    Language po_language = Language::from_env(filename->substr(0, filename->size()-3));

                    if (!po_language)
                    {
                        logMgr.logMessage(*filename + ": warning: ignoring, unknown language");
                    }
                    else
                    {
                        int score = Language::match(language, po_language);

                        if (score > best_score)
                        {
                            best_score = score;
                            best_filename = *filename;
                        }
                    }
                }
            }

            if (!best_filename.empty())
            {
                std::string pofile = *p + "/" + best_filename;
                try
                {
                    std::auto_ptr<std::istream> in = std::auto_ptr<std::istream>(new std::ifstream(pofile.c_str()));
                    if (!in.get())
                    {
                        logMgr.logMessage("error: failure opening: " + pofile, Ogre::LML_CRITICAL);
                    }
                    else
                    {
                        POParser::parse(pofile, *in, *dict);
                    }
                }
                catch(std::exception& e)
                {
                    logMgr.logMessage("error: failure parsing: "
                            + pofile + "\n" + e.what(), Ogre::LML_CRITICAL);
                }
            }
        }

        return *dict;
    }
}

std::set<Language> DictionaryManager::get_languages()
{
    std::set<Language> languages;

    for (SearchPath::iterator p = search_path.begin(); p != search_path.end(); ++p)
    {
        std::vector<std::string> files = ResourceManager::getSingleton().
                listAllFiles(*p);

        for(std::vector<std::string>::iterator file = files.begin(); file != files.end(); ++file)
        {
            if (ResourceManager::hasFileEnding(*file, ".po"))
            {
                languages.insert(Language::from_env(file->substr(0, file->size()-3)));
            }
        }
    }
    return languages;
}

void DictionaryManager::set_language(const Language& language)
{
    if (current_language != language)
    {
        current_language = language;
        current_dict     = 0;
    }
}

Language DictionaryManager::get_language() const
{
    return current_language;
}

void DictionaryManager::set_charset(const std::string& charset_)
{
    clear_cache(); // changing charset invalidates cache
    charset = charset_;
}

void DictionaryManager::set_use_fuzzy(bool t)
{
    clear_cache();
    use_fuzzy = t;
}

bool DictionaryManager::get_use_fuzzy() const
{
    return use_fuzzy;
}

void DictionaryManager::add_directory(const std::string& pathname)
{
    clear_cache(); // adding directories invalidates cache
    search_path.push_back(pathname);
}

/*
void DictionaryManager::set_filesystem(std::auto_ptr<FileSystem> filesystem_)
{
    filesystem = filesystem_;
}*/

std::string DictionaryManager::convertFilenameToLanguage(const std::string &s_in) const
{
    std::string s = ResourceManager::hasFileEnding(s_in, ".po")
                    ? s_in.substr(0, s_in.size()-3)
                    : s_in;

    bool underscore_found = false;
    for(unsigned int i = 0, size = s.size(); i < size; ++i)
    {
        if(underscore_found)
        {
            // If we get a non-alphanumerical character/
            // we are done (en_GB.UTF-8) - only convert
            // the 'gb' part ... if we ever get this kind
            // of filename.
            if(!::isalpha(s[i]))
                break;
            s[i] = ::toupper(s[i]);
        }
        else
            underscore_found = s[i]=='_';
    }

    return s;
}

} // namespace tinygettext

/* EOF */
