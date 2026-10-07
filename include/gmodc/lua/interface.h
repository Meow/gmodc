#ifndef GMODC_LUA_INTERFACE_H
#define GMODC_LUA_INTERFACE_H

/*
  Single include for a C binary module. Pulls in the C API (CCompat.h), the
  TYPE_* constants and the Vector/QAngle structs, and defines the entry point
  macros below.

    #include <gmodc/lua/interface.h>

    LUA_FUNCTION(my_function) {
      lua_push_string(LUA, "hello", 0);
      return 1;
    }

    GMOD_MODULE_OPEN() {
      lua_push_special(LUA, LUA_SPECIAL_GLOB);
      lua_push_cfunc(LUA, my_function);
      lua_set_field(LUA, -2, "my_function");
      lua_pop(LUA, 1);
      return 0;
    }

    GMOD_MODULE_CLOSE() { return 0; }
*/

#include <GarrysMod/Lua/CCompat.h>
#include "source_compat.h"
#include "types.h"
#include "lua.h"

/* Arguments for lua_push_special. */
enum {
  LUA_SPECIAL_GLOB, /*  Global table (_G)                                  */
  LUA_SPECIAL_ENV,  /*  Environment table of the running function          */
  LUA_SPECIAL_REG   /*  Registry table, private to C code                  */
};

#ifndef GMOD
#ifdef _WIN32
#define GMOD_DLL_EXPORT __declspec(dllexport)
#else
#define GMOD_DLL_EXPORT __attribute__((visibility("default")))
#endif

#ifdef GMOD_MODULE_OPEN
#undef GMOD_MODULE_OPEN
#endif

#ifdef GMOD_MODULE_CLOSE
#undef GMOD_MODULE_CLOSE
#endif

#ifdef LUA_FUNCTION
#undef LUA_FUNCTION
#endif

/*
  Defines the exported gmod13_open entry point that Garry's Mod calls when
  Lua does require("name"). Follow the macro with a function body; inside it
  `LUA` is the luabase_t handle. Return the number of values left on the
  stack, which become the results of require(). Usually 0.
*/
#define GMOD_MODULE_OPEN()                        \
  int gmod13_open__Imp(luabase_t *LUA);           \
  GMOD_DLL_EXPORT int gmod13_open(lua_State *L)   \
  {                                               \
    return gmod13_open__Imp(lua_get_base(L));     \
  }                                               \
  int gmod13_open__Imp(luabase_t *LUA)

/*
  Defines the exported gmod13_close entry point, called when the Lua state
  shuts down. Free references and C resources here. Return 0.
*/
#define GMOD_MODULE_CLOSE()                       \
  int gmod13_close__Imp(luabase_t *LUA);          \
  GMOD_DLL_EXPORT int gmod13_close(lua_State *L)  \
  {                                               \
    return gmod13_close__Imp(lua_get_base(L));    \
  }                                               \
  int gmod13_close__Imp(luabase_t *LUA)

/*
  Defines a function named FUNC that Lua can call (a cfunc_t). The macro
  binds the state and provides `LUA` inside the body. Arguments are at stack
  positions 1..lua_top(LUA); push the results and return how many there are.

    LUA_FUNCTION(add) {
      lua_push_number(LUA, lua_check_number(LUA, 1) + lua_check_number(LUA, 2));
      return 1;
    }
*/
#define LUA_FUNCTION(FUNC)                        \
  int FUNC##__Imp(luabase_t *LUA);                \
  int FUNC(lua_State *L)                          \
  {                                               \
    luabase_t *LUA = lua_get_base(L);             \
    lua_set_state(LUA, L);                        \
    return FUNC##__Imp(LUA);                      \
  }                                               \
  int FUNC##__Imp(luabase_t *LUA)
#endif

#endif
