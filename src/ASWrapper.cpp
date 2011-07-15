/*!
 * \file   ASWrapper.cpp
 * \date   08 July 2011
 * \author StefanP.MUC
 * \brief  Initializes Lua and provides access to its functions
 */

#include "angelscript.h"

#include "ASWrapper.h"

template<> ASWrapper* Ogre::Singleton<ASWrapper>::ms_Singleton = 0;

/*! \brief Initialises AngelScript
 *
 */
ASWrapper::ASWrapper() :
        scriptEngine(asCreateScriptEngine(ANGELSCRIPT_VERSION))
{

}

/*! \brief closes Lua
 *
 */
ASWrapper::~ASWrapper()
{
    scriptEngine->Release();
}
