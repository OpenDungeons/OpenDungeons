/*!
 * \file   ASWrapper.h
 * \date   08 July 2011
 * \author StefanP.MUC
 * \brief  Initializes AngelScript and provides access to its functions
 */

#ifndef ASWRAPPER_H_
#define ASWRAPPER_H_

#include <OgreSingleton.h>

class asIObjectType;
class asIScriptContext;
class asIScriptEngine;
class asIScriptModule;
class asSMessageInfo;
class CScriptBuilder;
class CameraManager;

/*! \brief This singleton initialises AngelScript and provides all neccessary functions to
 *         execute script code, including registering our own classes, templated reference
 *         classes and several helper functions.
 */
class ASWrapper : public Ogre::Singleton<ASWrapper>
{
    public:
        ASWrapper   (CameraManager*);
        ~ASWrapper  ();

        void executeScriptCode      (const std::string& code);
        void executeConsoleCommand  (const std::vector<std::string>& command);

    private:
        asIScriptEngine*    engine;
        CScriptBuilder*     builder;
        asIScriptContext*   context;
        asIObjectType*      stringArray;
	CameraManager*      cameraManager;

        void messageCallback    (const asSMessageInfo* msg, void* param);
        void registerEverything ();

        //! \brief Empty dummy ref function for objects that are not new/delete by the script
        static void dummy(void*) {}

        //! \brief Tenplated factory function
        template<class T>
        static T* createInstance(){return new T();}

        /*! \brief templated wrapper class providing factory, addref, release functions for AS.
         */
        template<class T>
        class ASRef : public T
        {
            public:
                //! \brief Set refCount to 0
                ASRef() : refCount(0) {}

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
