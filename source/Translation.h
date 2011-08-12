/*!
 * \file   Translation.h
 * \date   27 April 2011
 * \author StefanP.MUC
 * \brief  handles the translation
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
        Translation();
        ~Translation();

        inline const std::vector<std::string>* getLanguageList() const{return &languageList;}
        inline const std::vector<std::string>* getLanguageNiceNames() const{return &languageNiceNames;}
        inline std::string translate(const std::string& original) {return dictionary.translate(original);}

    private:
        Translation(const Translation&);

        std::vector<std::string> languageList;
        tinygettext::DictionaryManager dictMgr;
        tinygettext::Dictionary dictionary;
        std::vector<std::string> languageNiceNames;
};

#endif /* TRANSLATION_H_ */
