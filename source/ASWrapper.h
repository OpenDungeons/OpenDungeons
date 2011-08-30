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

/*! \brief This singleton initialises AngelScript and provides all neccessary functions to
 *         execute script code, including registering our own classes, templated reference
 *         classes and several helper functions.
 */
class ASWrapper : public Ogre::Singleton<ASWrapper>
{
    public:
        ASWrapper   ();
        ~ASWrapper  ();

        void executeScriptCode      (const std::string& code);
        void executeConsoleCommand  (const std::vector<std::string>& command);

    private:
        asIScriptEngine*    engine;
        asIScriptModule*    module;
        asIScriptContext*   context;
        asIObjectType*      stringArray;

        void messageCallback    (const asSMessageInfo* msg, void* param);
        void registerEverything ();
        void loadScript         (const std::string& fileName);

        //! \brief Empty dummy ref function for objects that are not new/delete by the script
        static void dummy(void*) {}

        //script helper functions
        static int          stringToInt     (const std::string& str);
        static unsigned int stringToUInt    (const std::string& str);
        static float        stringToFloat   (const std::string& str);
        static bool         checkIfInt      (const std::string& str);
        static bool         checkIfFloat    (const std::string& str);

        /*! \brief templated wrapper class providing factory, addref, release functions for AS.
         */
        template<typename T>
        class ASRef : public T
        {
            public:
                ASRef();

                //! \brief Factory function
                static T* createInstance(){return new T();}

                //! \brief Increase the reference counter
                void addref(){++refCount;}

                //! \brief Decrease ref count and delete if it reaches 0
                void release(){if(--refCount == 0){delete this;}}

                //! \brief Returns the current refCount
                const unsigned int& getRefCount() const {return refCount;}

            private:
                unsigned int refCount;
        };
};

#endif /* ASWRAPPER_H_ */
