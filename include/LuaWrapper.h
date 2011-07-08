/*!
 * \file   LuaWrapper.h
 * \date   08 July 2011
 * \author StefanP.MUC
 * \brief  Initializes Lua and provides access to its functions
 */

#ifndef LUAWRAPPER_H_
#define LUAWRAPPER_H_

#include <Ogre.h>

class lua_State;

class LuaWrapper :
    public Ogre::Singleton<LuaWrapper>
{
    public:
        LuaWrapper();
        ~LuaWrapper();

        inline lua_State* getLuaState() const{return luaState;}

        bool executeLuaFile(std::string filename);

    private:
        lua_State* luaState;
};


#endif /* LUAWRAPPER_H_ */
