/*!
 * \file   Translation.h
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

#ifndef TRANSLATION_H_
#define TRANSLATION_H_

#include <vector>
#include <string>

#include "tinygettext.hpp"

#define _(String) (Translation::getSingletonPtr()->translate(String))

class Translation : public Ogre::Singleton<Translation>
{
public:
    //! \brief Initializes tinygettext (loading available languages)
    Translation();
    ~Translation();

    inline const std::vector<std::string>& getLanguageList() const
    { return mLanguageList; }

    inline const std::vector<std::string>& getLanguageNiceNames() const
    { return mLanguageNiceNames; }

    inline std::string translate(const std::string& original)
    { return mDictionary.translate(original); }

private:
    std::vector<std::string> mLanguageList;
    tinygettext::DictionaryManager mDictMgr;
    tinygettext::Dictionary mDictionary;
    std::vector<std::string> mLanguageNiceNames;
};

#endif // TRANSLATION_H_
