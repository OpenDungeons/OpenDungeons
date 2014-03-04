/*!
 * \file   ASWrapper.h
 * \date   08 July 2011
 * \author StefanP.MUC
 * \brief  Initializes AngelScript and provides access to its functions
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

#ifndef ASWRAPPER_H_
#define ASWRAPPER_H_

#include <OgreSingleton.h>

class asIObjectType;
class asIScriptContext;
class asIScriptEngine;
class asIScriptModule;
struct asSMessageInfo;
class CScriptBuilder;
class CameraManager;

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
    asIScriptEngine*    mEngine;
    CScriptBuilder*     mBuilder;
    asIScriptContext*   mContext;
    asIObjectType*      mStringArray;

    void messageCallback    (const asSMessageInfo* msg, void* param);
    void registerEverything ();

    //! \brief Empty dummy ref function for objects that are not new/delete by the script
    static void dummy(void*) {}

    //! \brief Tenplated factory function
    template<class T>
    static T* createInstance()
    { return new T(); }

    //! \brief templated wrapper class providing factory, addref, release functions for AS.
    template<class T>
    class ASRef : public T
    {
    public:
        //! \brief Set refCount to 0
        ASRef():
            refCount(0)
        {}

        //! \brief Increase the reference counter
        void addref()
        { ++refCount; }

        //! \brief Decrease ref count and delete if it reaches 0
        void release(){
            if(--refCount == 0)
                delete this;
        }

        //! \brief Returns the current refCount
        const unsigned int& getRefCount() const
        { return refCount; }

    private:
        unsigned int refCount;
    };
};

#endif /* ASWRAPPER_H_ */
