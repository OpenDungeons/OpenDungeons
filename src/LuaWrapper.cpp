/*!
 * \file   LuaWrapper.cpp
 * \date   08 July 2011
 * \author StefanP.MUC
 * \brief  Initializes Lua and provides access to its functions
 */

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "LuaWrapper.h"

template<> LuaWrapper* Ogre::Singleton<LuaWrapper>::ms_Singleton = 0;

/*! \brief Initialises Lua
 *
 */
LuaWrapper::LuaWrapper() :
        luaState(lua_open())
{
    //open all standard libs
    luaL_openlibs(luaState);
}

/*! \brief closes Lua
 *
 */
LuaWrapper::~LuaWrapper()
{
    lua_close(luaState);
}

/*! \brief executes a .lua file
 *
 *  \param filename the Filename of the file to be executed
 *
 *  \return true if the file was found, false if the file was not found
 */
bool LuaWrapper::executeLuaFile(std::string filename)
{
    if(luaL_loadfile(luaState, filename.c_str()) == 0)
    {
        lua_pcall(luaState, 0, LUA_MULTRET, 0);
        return true;
    }

    return false;
}
