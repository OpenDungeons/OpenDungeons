/*!
 * \file   ASWrapper.h
 * \date   08 July 2011
 * \author StefanP.MUC
 * \brief  Initializes Lua and provides access to its functions
 */

#ifndef ASWRAPPER_H_
#define ASWRAPPER_H_

#include <Ogre.h>

class asIScriptEngine;

class ASWrapper :
    public Ogre::Singleton<ASWrapper>
{
    public:
        ASWrapper();
        ~ASWrapper();

    private:
        asIScriptEngine* scriptEngine;
};


#endif /* ASWRAPPER_H_ */
