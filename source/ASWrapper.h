/*!
 * \file   ASWrapper.h
 * \date   08 July 2011
 * \author StefanP.MUC
 * \brief  Initializes AngelScript and provides access to its functions
 */

#ifndef ASWRAPPER_H_
#define ASWRAPPER_H_

#include <Ogre.h>

class asIObjectType;
class asIScriptContext;
class asIScriptEngine;
class asIScriptModule;
class asSMessageInfo;

class ASWrapper :
    public Ogre::Singleton<ASWrapper>
{
    public:
        ASWrapper();
        ~ASWrapper();

        void loadScript(std::string fileName);
        void executeScriptCode(const std::string& code);
        void executeConsoleCommand(const std::vector<std::string>& command);

        // \brief helper function for registering the AS factories
        template<class T>
        static T* createInstance(){return new T();}

        // \brief dummy addref/release function
        static void dummyRef(){}

    private:
        asIScriptEngine*    engine;
        asIScriptModule*    module;
        asIScriptContext*   context;
        asIObjectType*      stringArray;

        void messageCallback(const asSMessageInfo* msg, void* param);
        void registerEverything();

        //script helper functions
        static int      stringToInt(const std::string& str);
        static float    stringToFloat(const std::string& str);
        static bool     checkIfInt(const std::string& str);
        static bool     checkIfFloat(const std::string& str);
};


#endif /* ASWRAPPER_H_ */
