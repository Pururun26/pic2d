#ifndef PICOLIB_LUA_H
#define PICOLIB_LUA_H

#include <lua.h>

// Регистрирует все функции picolib в Lua
void picolib_lua_register(lua_State *L);

#endif